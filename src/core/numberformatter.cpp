// SPDX-FileCopyrightText: 2013, 2015-2018, 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later


#include "core/numberformatter.h"

#include "core/complexform.h"
#include "core/regexpatterns.h"
#include "core/unitdisplayformat.h"
#include "core/settings.h"
#include "core/symbolicnumberformat.h"
#include "core/unicodechars.h"
#include "core/mathdsl.h"
#include "math/quantity.h"
#include "math/rational.h"
#include "core/units.h"

#include <algorithm>


static const QChar g_dotChar = MathDsl::MulDotOp;
static const QChar g_minusChar = MathDsl::SubOp;

namespace {

constexpr int RationalFormatMaxDenominator = 1000000;
// Internal sentinel set by rat()/ratio()/rational() via Quantity::Format::precision.
// It forces rational display for a single result, with automatic decimal fallback.
constexpr int ForcedRationalPrecision = -999;
HNumber rationalFormatRelativeTolerance()
{
    return HNumber("1e-60");
}

bool isCloseTo(const HNumber& value, const HNumber& reference)
{
    return SymbolicNumberFormat::isCloseToTrigSymbolicValue(value, reference);
}

void useUnprefixedSiUnitForScientificDisplay(Quantity* quantity)
{
    if (!quantity || !quantity->hasUnit())
        return;

    const auto baseIdForDisplayUnit = [](const QString& unitName) {
        const QString normalized = normalizeUnitName(unitName);
        const UnitId directId = unitId(normalized);
        if (directId != UnitId::Unknown)
            return directId;

        struct PrefixText {
            QString text;
            UnitPrefixSpec spec;
        };
        QList<PrefixText> prefixes;
        for (const UnitPrefixSpec& prefix : siPrefixes()) {
            prefixes.append(PrefixText{normalizeUnitName(prefix.symbol), prefix});
            prefixes.append(PrefixText{normalizeUnitName(prefix.longName), prefix});
        }
        std::sort(prefixes.begin(), prefixes.end(), [](const PrefixText& a, const PrefixText& b) {
            return a.text.size() > b.text.size();
        });

        for (const PrefixText& prefix : prefixes) {
            if (prefix.text.isEmpty()
                || !normalized.startsWith(prefix.text)
                || normalized.size() == prefix.text.size())
            {
                continue;
            }

            const UnitId baseId = unitId(normalized.mid(prefix.text.size()));
            const int policy = unitPrefixPolicy(baseId);
            if ((policy & AllDecimalSiPrefixes)
                && unitPrefixPolicyAllows(policy, prefix.spec))
            {
                return baseId;
            }
        }
        return UnitId::Unknown;
    };

    const UnitId id = baseIdForDisplayUnit(quantity->unitName());
    if (!(unitPrefixPolicy(id) & AllDecimalSiPrefixes))
        return;

    const QString baseSymbol = unitSiPrefixBaseSymbol(id);
    const CNumber baseValue = unitSiPrefixBaseValue(id);
    if (baseSymbol.isEmpty() || !baseValue.isNearReal() || baseValue.real.isNearZero())
        return;

    quantity->setDisplayUnit(baseValue, baseSymbol);
}

QString formatCommonTrigRadical(const HNumber& value)
{
    const bool negative = value.isNegative();
    const HNumber absValue = negative ? -value : value;

    const HNumber sqrt2 = HMath::sqrt(HNumber(2));
    const HNumber sqrt3 = HMath::sqrt(HNumber(3));

    QString symbol;
    if (isCloseTo(absValue, sqrt2 / HNumber(2)))
        symbol = SymbolicNumberFormat::formatFraction(QStringLiteral("sqrt(2)"), 2);
    else if (isCloseTo(absValue, sqrt3 / HNumber(2)))
        symbol = SymbolicNumberFormat::formatFraction(QStringLiteral("sqrt(3)"), 2);
    else if (isCloseTo(absValue, sqrt3 / HNumber(3)))
        symbol = SymbolicNumberFormat::formatFraction(QStringLiteral("sqrt(3)"), 3);
    else if (isCloseTo(absValue, HNumber(2) * sqrt3 / HNumber(3)))
        symbol = SymbolicNumberFormat::formatFraction(QStringLiteral("2 sqrt(3)"), 3);
    else if (isCloseTo(absValue, sqrt2))
        symbol = QStringLiteral("sqrt(2)");
    else if (isCloseTo(absValue, sqrt3))
        symbol = QStringLiteral("sqrt(3)");

    if (symbol.isEmpty())
        return QString();
    return negative ? QStringLiteral("-") + symbol : symbol;
}

QString formatCommonTrigFraction(const HNumber& value)
{
    const bool negative = value.isNegative();
    const HNumber absValue = negative ? -value : value;

    QString symbol;
    if (isCloseTo(absValue, HNumber(1) / HNumber(2)))
        symbol = SymbolicNumberFormat::formatFraction(QStringLiteral("1"), 2);

    if (symbol.isEmpty())
        return QString();
    return negative ? QStringLiteral("-") + symbol : symbol;
}

QString formatTrigSymbolicDisplay(const Quantity& q, const HNumber& value)
{
    if (!q.isDimensionless() || !q.unitName().isEmpty())
        return QString();

    QString symbolic = SymbolicNumberFormat::formatPiMultiple(value);
    if (!symbolic.isEmpty())
        return symbolic;

    symbolic = formatCommonTrigFraction(value);
    if (!symbolic.isEmpty())
        return symbolic;

    return formatCommonTrigRadical(value);
}

QString formatRationalDisplay(Quantity q)
{
    if (q.isNan())
        return QString();

    if (!q.hasUnit() && !q.isDimensionless()) {
        q.cleanDimension();
        Units::findUnit(q);
    }

    CNumber number = q.numericValue();
    number /= q.unit();
    if (!number.isNearReal())
        return QString();

    QString symbolicTrig = formatTrigSymbolicDisplay(q, number.real);
    if (!symbolicTrig.isEmpty())
        return symbolicTrig;

    Rational rational;
    if (!Rational::approximate(number.real, RationalFormatMaxDenominator,
            rationalFormatRelativeTolerance(), &rational)) {
        return QString();
    }

    QString result;
    if (rational.denominator() == 1) {
        result = QString::number(rational.numerator());
    } else {
        result = SymbolicNumberFormat::formatFraction(QString::number(rational.numerator()), rational.denominator());
    }

    const QString unitName = q.unitName();
    if (!unitName.isEmpty())
        result += QStringLiteral("[%1]").arg(unitName);
    return result;
}

bool isValidDigitForBase(const QChar ch, int base)
{
    if (base <= 10)
        return ch >= QLatin1Char('0') && ch < QLatin1Char('0' + base);

    if (ch >= QLatin1Char('0') && ch <= QLatin1Char('9'))
        return true;

    const QChar lower = ch.toLower();
    return lower >= QLatin1Char('a') && lower < QLatin1Char('a' + (base - 10));
}

bool isValidGroupedIntegerPart(const QString& part, QChar separator, int base, int groupSize)
{
    const QStringList groups = part.split(separator, Qt::KeepEmptyParts);
    if (groups.isEmpty())
        return false;

    for (int i = 0; i < groups.size(); ++i) {
        const QString& group = groups.at(i);
        if (group.isEmpty())
            return false;
        const int expectedSize = (i == 0) ? -1 : groupSize;
        if (expectedSize > 0 && group.size() != expectedSize)
            return false;
        if (i == 0 && group.size() > groupSize)
            return false;
        for (const QChar ch : group) {
            if (!isValidDigitForBase(ch, base))
                return false;
        }
    }

    return true;
}

bool canonicalizeLiteralMantissa(const QString& input, int base, int groupingSize, QString* output)
{
    const int dotCount = input.count(MathDsl::DotSep);
    const int commaCount = input.count(MathDsl::CommaSep);

    if (dotCount == 0 && commaCount == 0) {
        if (input.isEmpty())
            return false;
        for (const QChar ch : input) {
            if (!isValidDigitForBase(ch, base))
                return false;
        }
        *output = input;
        return true;
    }

    QChar radixSep;
    QChar groupingSep;
    bool hasRadix = false;
    if (dotCount > 0 && commaCount > 0) {
        const int lastDot = input.lastIndexOf(MathDsl::DotSep);
        const int lastComma = input.lastIndexOf(MathDsl::CommaSep);
        hasRadix = true;
        radixSep = (lastDot > lastComma) ? MathDsl::DotSep : MathDsl::CommaSep;
        if (base == 10 && NumberFormatter::hasFormattedFractionalGrouping(input))
            radixSep = QChar(Settings::instance()->decimalSeparator());
        groupingSep = (radixSep == MathDsl::DotSep) ? MathDsl::CommaSep : MathDsl::DotSep;
        if (input.count(radixSep) > 1)
            return false;
    } else {
        const QChar sep = (dotCount > 0) ? MathDsl::DotSep : MathDsl::CommaSep;
        const int sepCount = input.count(sep);
        if (sepCount == 1) {
            // Pasting a standalone value must use the same separator rules
            // as typing it, including the mantissa of scientific notation.
            const bool looksGroupedInteger = base == 10
                && NumberFormatter::isSingleGroupedInteger(input);
            if (looksGroupedInteger) {
                hasRadix = false;
                groupingSep = sep;
            } else {
                hasRadix = true;
                radixSep = sep;
            }
        } else {
            hasRadix = false;
            groupingSep = sep;
            if (!isValidGroupedIntegerPart(input, groupingSep, base, groupingSize))
                return false;
        }
    }

    if (!hasRadix) {
        if (!NumberFormatter::hasValidNumberGrouping(input, QChar(), base))
            return false;
        QString integral = input;
        if (!groupingSep.isNull())
            integral.remove(groupingSep);
        if (integral.isEmpty())
            return false;
        for (const QChar ch : integral) {
            if (!isValidDigitForBase(ch, base))
                return false;
        }
        *output = integral;
        return true;
    }

    const int radixPos = input.lastIndexOf(radixSep);
    if (!NumberFormatter::hasValidNumberGrouping(input, radixSep, base))
        return false;
    if (radixPos <= 0 || radixPos >= input.size() - 1)
        return false;

    QString integral = input.left(radixPos);
    QString fractional = input.mid(radixPos + 1);
    if (integral.isEmpty() || fractional.isEmpty())
        return false;

    if (!groupingSep.isNull()) {
        if (!isValidGroupedIntegerPart(integral, groupingSep, base, groupingSize))
            return false;
        integral.remove(groupingSep);
        fractional.remove(groupingSep);
    }

    for (const QChar ch : integral) {
        if (!isValidDigitForBase(ch, base))
            return false;
    }
    for (const QChar ch : fractional) {
        if (!isValidDigitForBase(ch, base))
            return false;
    }

    *output = integral + MathDsl::DotSep + fractional;
    return true;
}

bool canonicalizeStandaloneNumericLiteral(const QString& input, QString* output)
{
    QString trimmed = input.trimmed();
    if (trimmed.isEmpty())
        return false;

    QString sign;
    if (trimmed.startsWith(MathDsl::AddOp)) {
        trimmed.remove(0, 1);
    } else if (trimmed.startsWith(MathDsl::SubOpAl1)
               || trimmed.startsWith(g_minusChar)) {
        sign = QStringLiteral("-");
        trimmed.remove(0, 1);
    }
    if (trimmed.isEmpty())
        return false;

    QString mantissa = trimmed;
    QString exponent;
    int base = 10;
    int groupingSize = 3;

    QString prefix;
    if (trimmed.startsWith(MathDsl::HexPrefix, Qt::CaseInsensitive)) {
        base = 16;
        groupingSize = 4;
        prefix = trimmed.left(2);
        mantissa = trimmed.mid(2);
    } else if (trimmed.startsWith(MathDsl::OctPrefix, Qt::CaseInsensitive)) {
        base = 8;
        groupingSize = 3;
        prefix = trimmed.left(2);
        mantissa = trimmed.mid(2);
    } else if (trimmed.startsWith(MathDsl::BinPrefix, Qt::CaseInsensitive)) {
        base = 2;
        groupingSize = 4;
        prefix = trimmed.left(2);
        mantissa = trimmed.mid(2);
    } else {
        const int exponentPos = mantissa.indexOf(RegExpPatterns::decimalExponentSuffix());
        if (exponentPos >= 0) {
            exponent = mantissa.mid(exponentPos);
            mantissa = mantissa.left(exponentPos);
        }
    }

    if (mantissa.isEmpty())
        return false;

    QString canonicalMantissa;
    if (!canonicalizeLiteralMantissa(mantissa, base, groupingSize, &canonicalMantissa)) {
        return false;
    }

    QString canonical = sign + prefix + canonicalMantissa + exponent;
    if (base == 10) {
        const QByteArray bytes = canonical.toLatin1();
        const char* rest = nullptr;
        const HNumber parsed = HMath::parse_str(bytes.constData(), &rest);
        if (parsed.isNan() || !rest || *rest != '\0')
            return false;
    }

    *output = canonical;
    return true;
}

} // namespace

QString NumberFormatter::format(Quantity q)
{
    return format(q, '\0');
}

QString NumberFormatter::formatTrigSymbolic(Quantity q)
{
    if (q.isNan())
        return QString();

    CNumber number = q.numericValue();
    number /= q.unit();
    if (!number.isNearReal())
        return QString();

    return formatTrigSymbolicDisplay(q, number.real);
}

QString NumberFormatter::format(Quantity q, char resultFormatOverride)
{
    Settings* settings = Settings::instance();
    return format(q,
                  resultFormatOverride,
                  settings->resultPrecision,
                  settings->complexNumbers,
                  settings->resultComplexForm);
}

QString NumberFormatter::format(Quantity q, char resultFormatOverride,
                                int precisionOverride, bool useComplexNotation,
                                char complexNotationOverride)
{
    Settings* settings = Settings::instance();
    CMath::setImaginaryUnitSymbol(settings->imaginaryUnit);
    CMath::setPolarAngleUnit(settings->angleUnit);
    const char activeResultFormat = resultFormatOverride == '\0' ? settings->resultFormat : resultFormatOverride;
    QString result;

    Quantity::Format format = q.format();
    // Check per-expression rational-display override encoded in format precision.
    const bool forceRationalDisplay = format.precision == ForcedRationalPrecision;
    const bool hasExplicitFormatOverride =
        format.base != Quantity::Format::Base::Null
        || format.mode != Quantity::Format::Mode::Null;
    if (((!hasExplicitFormatOverride && activeResultFormat == 'r') || forceRationalDisplay)
            && format.base != Quantity::Format::Base::Binary
            && format.base != Quantity::Format::Base::Octal
            && format.base != Quantity::Format::Base::Hexadecimal
            && format.mode != Quantity::Format::Mode::Sexagesimal) {
        result = formatRationalDisplay(q);
    }

    if (format.base == Quantity::Format::Base::Null) {
        switch (activeResultFormat) {
        case 'b':
            format.base = Quantity::Format::Base::Binary;
            break;
        case 'o':
            format.base = Quantity::Format::Base::Octal;
            break;
        case 'h':
            format.base = Quantity::Format::Base::Hexadecimal;
            break;
        case 'n':
        case 'f':
        case 'e':
        case 'r':
        case 's':
        case 'g':
        default:
            format.base = Quantity::Format::Base::Decimal;
            break;
        }
    }

    if (format.mode == Quantity::Format::Mode::Null) {
        if (format.base == Quantity::Format::Base::Decimal) {
          switch (activeResultFormat) {
          case 'n':
              format.mode = Quantity::Format::Mode::Engineering;
              break;
          case 'f':
              format.mode = Quantity::Format::Mode::Fixed;
              break;
          case 'e':
              format.mode = Quantity::Format::Mode::Scientific;
              break;
          case 's':
              format.mode = Quantity::Format::Mode::Sexagesimal;
              break;
          case 'r':
          case 'g':
          default:
              format.mode = Quantity::Format::Mode::General;
              break;
          }
        } else {
            format.mode = Quantity::Format::Mode::Fixed;
        }
    }

    if (format.precision == Quantity::Format::PrecisionNull)
        format.precision = precisionOverride;
    format.removeTrailingZeroes = settings->removeTrailingZeroes;
    if (format.notation == Quantity::Format::Notation::Null) {
        if (useComplexNotation) {
            if (complexNotationOverride == ComplexForm::Rectangular)
                format.notation = Quantity::Format::Notation::Cartesian;
            else if (complexNotationOverride == ComplexForm::Exponential)
                format.notation = Quantity::Format::Notation::Polar;
            else if (complexNotationOverride == ComplexForm::Trigonometric)
                format.notation = Quantity::Format::Notation::Trigonometric;
            else if (complexNotationOverride == ComplexForm::Cis)
                format.notation = Quantity::Format::Notation::Cis;
            else if (complexNotationOverride == ComplexForm::Phasor)
                format.notation = Quantity::Format::Notation::PolarAngle;
        }
    }

    if (format.base == Quantity::Format::Base::Decimal
        && (format.mode == Quantity::Format::Mode::Scientific || activeResultFormat == 'e'))
    {
        if (!q.hasUnit() && !q.isDimensionless()) {
            q.cleanDimension();
            Units::findUnit(q);
        }
        useUnprefixedSiUnitForScientificDisplay(&q);
    }

    const bool useSexagesimalOutput =
        format.base == Quantity::Format::Base::Decimal
        && format.mode == Quantity::Format::Mode::Sexagesimal;

    bool time = false, arc = q.isDimensionless();
    if (useSexagesimalOutput && q.hasDimension()) {
        auto dimension = q.getDimensionByQuantity();
        if (dimension.count() == 1 && dimension.firstKey() == UnitQuantity::Time) {
            auto iterator = dimension.begin();
            if ( iterator->numerator() == 1 && iterator->denominator() == 1) {
                q.stripUnits();
                q.clearDimension(); // remove unit, formatting itself is unit
                time = true;
            }
        }
    }

    if (arc && useSexagesimalOutput) {     // convert to arcseconds
        // Sexagesimal angle formatting is unitless "D°M′S″" output. If the
        // value carries an explicit angle display unit (e.g. [rad]), drop it
        // before numeric formatting/parsing to avoid contaminating the
        // intermediate scalar with unit text.
        q.stripUnits();
        if (settings->angleUnit == 'r')
            q /= Quantity(HMath::pi() / HNumber(180));
        else if (settings->angleUnit == 'g')
            q /= Quantity(HNumber(200) / HNumber(180));
        else if (settings->angleUnit == 't' || settings->angleUnit == 'v')
            q *= Quantity(360);
        q *= Quantity(3600);
    }

    bool negative = false;
    if (useSexagesimalOutput && q.isNegative()) {
        q *= Quantity(HNumber(-1));
        negative = true;
    }

    if (result.isEmpty())
        result = DMath::format(q, format);

    if (useSexagesimalOutput && (arc || time)) {   // sexagesimal
        int dotPos = result.indexOf(MathDsl::DotSep);
        HNumber seconds(dotPos > 0 ? result.left(dotPos).toStdString().c_str() : result.toStdString().c_str());
        HNumber mains = HMath::floor(seconds / HNumber(3600));
        seconds -= (mains * HNumber(3600));
        HNumber minutes = HMath::floor(seconds / HNumber(60));
        seconds -= (minutes * HNumber(60));
        HNumber::Format fixed = HNumber::Format::Fixed();
        QString sexa = HMath::format(mains, fixed);
        sexa.append(time ? MathDsl::TimeSep : MathDsl::Deg).append(minutes < 10 ? "0" : "").append(HMath::format(minutes, fixed));
        sexa.append(time ? MathDsl::TimeSep : MathDsl::ArcminOp).append(seconds < 10 ? "0" : "").append(HMath::format(seconds, fixed));
        if (dotPos > 0)     // append decimals
            sexa.append(result.mid(dotPos));
        if (!time)
            sexa.append(MathDsl::ArcsecOp);
        result = sexa;
    }

    if (negative)
        result.insert(0, MathDsl::SubOpAl1);

    if (settings->decimalSeparator() == MathDsl::CommaSep.toLatin1())
        result.replace(MathDsl::DotSep, MathDsl::CommaSep);

    if (q.hasUnit() || !q.isDimensionless()) {
        bool unitNormalized = false;
        const int openBracket = result.lastIndexOf(MathDsl::UnitStart);
        const int closeBracket = result.lastIndexOf(MathDsl::UnitEnd);
        if (openBracket >= 0
            && closeBracket > openBracket
            && closeBracket == result.size() - 1)
        {
            const QString normalizedUnit =
                UnitDisplayFormat::normalizeUnitTextForDisplay(
                    result.mid(openBracket + 1, closeBracket - openBracket - 1));
            result = result.left(openBracket + 1)
                + normalizedUnit
                + MathDsl::UnitEnd;
            unitNormalized = true;
        }

        if (!unitNormalized) {
            const int firstSpace = result.indexOf(QLatin1Char(' '));
            const int lastSpace = result.lastIndexOf(QLatin1Char(' '));
            int splitSpace = firstSpace;
            if (firstSpace >= 0 && lastSpace > firstSpace) {
                const QString between = result.mid(firstSpace + 1, lastSpace - firstSpace - 1).trimmed();
                const bool isScientificMantissaTail =
                    (between.startsWith(MathDsl::MulCrossOp)
                     || between.startsWith(MathDsl::MulOpAl1))
                    && between.contains(QStringLiteral("10"));
                if (isScientificMantissaTail)
                    splitSpace = lastSpace;
            }
            if (splitSpace >= 0 && splitSpace + 1 < result.size()) {
                const QString normalizedUnit =
                    UnitDisplayFormat::normalizeUnitTextForDisplay(result.mid(splitSpace + 1));
                result = result.left(splitSpace)
                    + MathDsl::UnitStart
                    + normalizedUnit
                    + MathDsl::UnitEnd;
            }
        }
    }

    result.replace(MathDsl::SubOpAl1, g_minusChar);

    // Replace all spaces between units with dot operator.
    int emptySpaces = 0;
    for (auto& ch : result) {
        if (ch.isSpace()) {
            ++emptySpaces;
            if (emptySpaces > 1)
                ch = g_dotChar;
        } else {
            emptySpaces = 0;
        }
    }

    return result;
}

bool NumberFormatter::isSingleGroupedInteger(const QString& input)
{
    // Grouped input remains valid even when the display uses spaces,
    // underscores, or no grouping. The selected decimal mark takes priority.
    const QString separator = Settings::instance()->decimalSeparator() == MathDsl::DotSep.toLatin1()
        ? QString(MathDsl::CommaSep) : QString(MathDsl::DotSep);
    const int separatorPos = input.indexOf(separator);
    if (separatorPos < 0 || input.size() - separatorPos - 1 != 3)
        return false;

    int start = input.startsWith(QLatin1Char('+')) || input.startsWith(QLatin1Char('-')) ? 1 : 0;
    if (input.mid(start, 2).compare(QStringLiteral("0d"), Qt::CaseInsensitive) == 0)
        start += 2;
    if (separatorPos <= start || separatorPos - start > 3)
        return false;
    for (int i = start; i < input.size(); ++i) {
        if (i != separatorPos && !input.at(i).isDigit())
            return false;
    }
    return true;
}

bool NumberFormatter::hasFormattedFractionalGrouping(const QString& input)
{
    const Settings* settings = Settings::instance();
    const QString separator = settings->digitGroupingSeparator();
    const QChar decimal(settings->decimalSeparator());
    if (settings->digitGroupingIntegerPartOnly
        || (separator != QString(MathDsl::DotSep) && separator != QString(MathDsl::CommaSep))
        || input.count(decimal) != 1)
        return false;

    const int radixPos = input.indexOf(decimal);
    int start = input.startsWith(QLatin1Char('+')) || input.startsWith(QLatin1Char('-')) ? 1 : 0;
    if (input.mid(start, 2).compare(QStringLiteral("0d"), Qt::CaseInsensitive) == 0)
        start += 2;
    const QString integral = input.mid(start, radixPos - start);
    if (integral.contains(separator)) {
        if (!isValidGroupedIntegerPart(integral, separator.at(0), 10, 3))
            return false;
    } else {
        if (integral.isEmpty())
            return false;
        for (QChar ch : integral) {
            if (!isValidDigitForBase(ch, 10))
                return false;
        }
    }

    const QStringList groups = input.mid(radixPos + 1).split(separator, Qt::KeepEmptyParts);
    if (groups.size() < 2)
        return false;
    for (int i = 0; i < groups.size(); ++i) {
        const QString& group = groups.at(i);
        if (group.isEmpty() || group.size() > 3 || (i < groups.size() - 1 && group.size() != 3))
            return false;
        for (QChar ch : group) {
            if (!isValidDigitForBase(ch, 10))
                return false;
        }
    }
    return true;
}

bool NumberFormatter::hasValidNumberGrouping(const QString& input, QChar radix, int base)
{
    QString body = input;
    if (body.startsWith(QLatin1Char('+')) || body.startsWith(QLatin1Char('-')))
        body.remove(0, 1);
    if (base == 10 && body.startsWith(QLatin1Char('#'))) {
        base = 16;
        body.remove(0, 1);
    } else if (base == 10 && body.size() >= 2 && body.at(0) == QLatin1Char('0')) {
        const QChar prefix = body.at(1).toLower();
        if (prefix == QLatin1Char('x') || prefix == QLatin1Char('b')
            || prefix == QLatin1Char('o') || prefix == QLatin1Char('d')) {
            base = prefix == QLatin1Char('x') ? 16 : prefix == QLatin1Char('b') ? 2
                : prefix == QLatin1Char('o') ? 8 : 10;
            body.remove(0, 2);
        }
    }
    if (base == 10) {
        const int exponent = body.indexOf(QLatin1Char('e'), 0, Qt::CaseInsensitive);
        if (exponent >= 0)
            body.truncate(exponent);
    }

    if (!body.contains(MathDsl::DotSep) && !body.contains(MathDsl::CommaSep))
        return true;
    const QChar grouping = radix.isNull()
        ? (body.contains(MathDsl::DotSep) ? MathDsl::DotSep : MathDsl::CommaSep)
        : (radix == MathDsl::DotSep ? MathDsl::CommaSep : MathDsl::DotSep);
    const int groupSize = base == 2 || base == 16 ? 4 : 3;
    const auto digits = [base](const QString& part) {
        return std::all_of(part.cbegin(), part.cend(), [base](QChar ch) {
            return isValidDigitForBase(ch, base);
        });
    };
    const auto integralGroups = [&](const QString& part) {
        if (!part.contains(grouping))
            return digits(part);
        if (isValidGroupedIntegerPart(part, grouping, base, groupSize))
            return true;
        // Indian grouping is also accepted in copied decimal values.
        const QStringList groups = part.split(grouping, Qt::KeepEmptyParts);
        if (base != 10 || groups.size() < 2 || groups.first().isEmpty()
            || groups.first().size() > 2 || groups.last().size() != 3)
            return false;
        for (int i = 0; i < groups.size(); ++i) {
            if (!digits(groups.at(i)) || (i > 0 && i < groups.size() - 1 && groups.at(i).size() != 2))
                return false;
        }
        return true;
    };
    if (radix.isNull())
        return integralGroups(body);
    if (body.count(radix) != 1)
        return false;
    const int point = body.indexOf(radix);
    if (!integralGroups(body.left(point)))
        return false;
    const QString fractional = body.mid(point + 1);
    if (!fractional.contains(grouping))
        return digits(fractional);
    const QStringList groups = fractional.split(grouping, Qt::KeepEmptyParts);
    for (int i = 0; i < groups.size(); ++i) {
        const QString& group = groups.at(i);
        if (group.isEmpty() || group.size() > groupSize
            || (i < groups.size() - 1 && group.size() != groupSize) || !digits(group))
            return false;
    }
    return true;
}

QList<int> NumberFormatter::digitGroupingPositions(int digitCount, int groupSize, bool fractional)
{
    // Share the grouping boundaries between concrete result text and the
    // editor's painted separators so both follow the same Number Format.
    const Settings* settings = Settings::instance();
    QList<int> positions;
    if (!settings->isDigitGroupingEnabled() || groupSize <= 0
            || (fractional && settings->digitGroupingIntegerPartOnly))
        return positions;
    if (fractional) {
        for (int pos = groupSize; pos < digitCount; pos += groupSize)
            positions.append(pos);
    } else {
        const int leadingGroupSize = settings->isIndianDigitGrouping() && groupSize == 3 ? 2 : groupSize;
        for (int pos = digitCount - groupSize; pos > 0; pos -= leadingGroupSize)
            positions.prepend(pos);
    }
    return positions;
}

QString NumberFormatter::formatNumericLiteralForDisplay(const QString& input)
{
    const Settings* settings = Settings::instance();
    const QString separator = settings->digitGroupingSeparator();
    const bool groupingEnabled = settings->isDigitGroupingEnabled() && !separator.isEmpty();
    const QChar decimalSep = QChar(settings->decimalSeparator());
    const QChar altDecimalSep = (decimalSep == MathDsl::DotSep) ? MathDsl::CommaSep : MathDsl::DotSep;

    auto groupPart = [&separator](const QString& digits, int groupSize, bool fromRight) {
        QString result;
        int start = 0;
        for (int pos : NumberFormatter::digitGroupingPositions(digits.size(), groupSize, !fromRight)) {
            result += digits.mid(start, pos - start) + separator;
            start = pos;
        }
        result += digits.mid(start);
        return result;
    };

    QString output;
    int lastPos = 0;
    auto it = RegExpPatterns::numericToken().globalMatch(input);
    while (it.hasNext()) {
        const auto match = it.next();
        output += input.mid(lastPos, match.capturedStart() - lastPos);

        QString token = match.captured(0);
        int prefixLength = 0;
        int groupSize = 3;
        if (token.startsWith(MathDsl::HexPrefix, Qt::CaseInsensitive)) {
            prefixLength = 2;
            groupSize = 4;
        } else if (token.startsWith(MathDsl::OctPrefix, Qt::CaseInsensitive)) {
            prefixLength = 2;
            groupSize = 3;
        } else if (token.startsWith(MathDsl::BinPrefix, Qt::CaseInsensitive)) {
            prefixLength = 2;
            groupSize = 4;
        }

        const QString prefix = token.left(prefixLength);
        QString body = token.mid(prefixLength);
        QString exponent;
        if (prefixLength == 0) {
            const int exponentPos = body.indexOf(RegExpPatterns::decimalExponentSuffix());
            if (exponentPos >= 0) {
                exponent = body.mid(exponentPos);
                exponent[0] = QLatin1Char('e');
                body = body.left(exponentPos);
            }
        }

        int radixPos = body.lastIndexOf(decimalSep);
        if (radixPos < 0)
            radixPos = body.lastIndexOf(altDecimalSep);

        auto stripGroupingChars = [&](const QString& part) {
            QString stripped;
            stripped.reserve(part.size());
            for (const QChar c : part) {
                if (c.isDigit() || c.isLetter())
                    stripped.append(c);
            }
            return stripped;
        };

        if (radixPos >= 0) {
            const QString integral = stripGroupingChars(body.left(radixPos));
            const QString fractional = stripGroupingChars(body.mid(radixPos + 1));
            QString groupedFractional = fractional;
            if (groupingEnabled && !settings->digitGroupingIntegerPartOnly)
                groupedFractional = groupPart(fractional, groupSize, false);
            token = prefix
                + (groupingEnabled ? groupPart(integral, groupSize, true) : integral)
                + decimalSep
                + groupedFractional
                + exponent;
        } else {
            const QString integral = stripGroupingChars(body);
            token = prefix + (groupingEnabled ? groupPart(integral, groupSize, true) : integral) + exponent;
        }

        output += token;
        lastPos = match.capturedEnd();
    }
    output += input.mid(lastPos);
    return output;
}

QString NumberFormatter::rewriteScientificNotationForDisplay(const QString& input)
{
    QString output;
    int lastPos = 0;
    auto it = RegExpPatterns::numericToken().globalMatch(input);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        output += input.mid(lastPos, match.capturedStart() - lastPos);

        const QString token = match.captured(0);
        QString rewritten = token;
        if (!token.startsWith(MathDsl::HexPrefix, Qt::CaseInsensitive)
            && !token.startsWith(MathDsl::OctPrefix, Qt::CaseInsensitive)
            && !token.startsWith(MathDsl::BinPrefix, Qt::CaseInsensitive)) {
            const int exponentPos = token.indexOf(RegExpPatterns::decimalExponentSuffix());
            if (exponentPos > 0) {
                const QString mantissa = token.left(exponentPos);
                QString exponent = token.mid(exponentPos + 1);

                bool negativeExponent = false;
                if (!exponent.isEmpty()) {
                    if (exponent.at(0) == MathDsl::AddOp) {
                        exponent.remove(0, 1);
                    } else if (exponent.at(0) == MathDsl::SubOpAl1
                               || exponent.at(0) == UnicodeChars::MinusSign) {
                        negativeExponent = true;
                        exponent.remove(0, 1);
                    }
                }

                QString superscript;
                if (negativeExponent)
                    superscript += MathDsl::PowNeg;
                for (const QChar ch : exponent) {
                    const QChar superscriptDigit = MathDsl::asciiDigitToSuperscript(ch);
                    if (superscriptDigit.isNull()) {
                        superscript.clear();
                        break;
                    }
                    superscript += superscriptDigit;
                }

                if (!superscript.isEmpty()) {
                    rewritten = mantissa
                        + QString(MathDsl::MulCrossWrapSp)
                        + MathDsl::MulCrossOp
                        + QString(MathDsl::MulCrossWrapSp)
                        + QStringLiteral("10")
                        + superscript;
                }
            }
        }

        output += rewritten;
        lastPos = match.capturedEnd();
    }
    output += input.mid(lastPos);
    return output;
}

bool NumberFormatter::tryFormatStandaloneNumericLiteralForDisplay(const QString& input, QString* output)
{
    if (!output)
        return false;

    QString canonical;
    if (!canonicalizeStandaloneNumericLiteral(input, &canonical))
        return false;

    *output = formatNumericLiteralForDisplay(canonical);
    return true;
}
