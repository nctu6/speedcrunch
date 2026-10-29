// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef TESTS_INTERPRETEDNUMBERCASES_H
#define TESTS_INTERPRETEDNUMBERCASES_H

#include "decimalinputcases.h"
#include "numberformatcases.h"

#include <QString>
#include <vector>

namespace InterpretedNumberCases {

enum Literal {
    LeadingZeroInteger, LeadingZeroFraction, LongLeadingGroup, Thousand,
    OneFraction, ThousandFraction, SmallFraction, ThousandZeroFraction,
    SmallZeroFraction, LongFraction, Million, ScientificInteger, ScientificFraction,
    LiteralCount
};

inline constexpr std::array<const char*, LiteralCount> literals = {{
    "0123", "0.123", "1234.567", "1000", "1.000", "1234.56", "1.23456",
    "1000.000", "1.000000", "1234.56789", "1234567", "1234E2", "1.234E2"
}};

// Literal display expectations follow the same dialog order as NumberFormatCases.
// Leading and trailing zeroes belong to the expression, even when the result
// drops them. Scientific notation must remain an expression rather than a result.
inline constexpr std::array<std::array<const char*, LiteralCount>, 15> displays = {{
    {{"0123", "0.123", "1234.567", "1000", "1.000", "1234.56", "1.23456", "1000.000", "1.000000", "1234.56789", "1234567", "1234e2", "1.234e2"}},
    {{"0123", "0,123", "1234,567", "1000", "1,000", "1234,56", "1,23456", "1000,000", "1,000000", "1234,56789", "1234567", "1234e2", "1,234e2"}},
    {{"0,123", "0.123", "1,234.567", "1,000", "1.000", "1,234.56", "1.23456", "1,000.000", "1.000000", "1,234.56789", "1,234,567", "1,234e2", "1.234e2"}},
    {{"0.123", "0,123", "1.234,567", "1.000", "1,000", "1.234,56", "1,23456", "1.000,000", "1,000000", "1.234,56789", "1.234.567", "1.234e2", "1,234e2"}},
    {{"0 123", "0.123", "1 234.567", "1 000", "1.000", "1 234.56", "1.23456", "1 000.000", "1.000000", "1 234.56789", "1 234 567", "1 234e2", "1.234e2"}},
    {{"0 123", "0,123", "1 234,567", "1 000", "1,000", "1 234,56", "1,23456", "1 000,000", "1,000000", "1 234,56789", "1 234 567", "1 234e2", "1,234e2"}},
    {{"0 123", "0.123", "1 234.567", "1 000", "1.000", "1 234.56", "1.234 56", "1 000.000", "1.000 000", "1 234.567 89", "1 234 567", "1 234e2", "1.234e2"}},
    {{"0 123", "0,123", "1 234,567", "1 000", "1,000", "1 234,56", "1,234 56", "1 000,000", "1,000 000", "1 234,567 89", "1 234 567", "1 234e2", "1,234e2"}},
    {{"0,123", "0.123", "1,234.567", "1,000", "1.000", "1,234.56", "1.234,56", "1,000.000", "1.000,000", "1,234.567,89", "1,234,567", "1,234e2", "1.234e2"}},
    {{"0.123", "0,123", "1.234,567", "1.000", "1,000", "1.234,56", "1,234.56", "1.000,000", "1,000.000", "1.234,567.89", "1.234.567", "1.234e2", "1,234e2"}},
    {{"0_123", "0.123", "1_234.567", "1_000", "1.000", "1_234.56", "1.23456", "1_000.000", "1.000000", "1_234.56789", "1_234_567", "1_234e2", "1.234e2"}},
    {{"0_123", "0,123", "1_234,567", "1_000", "1,000", "1_234,56", "1,23456", "1_000,000", "1,000000", "1_234,56789", "1_234_567", "1_234e2", "1,234e2"}},
    {{"0_123", "0.123", "1_234.567", "1_000", "1.000", "1_234.56", "1.234_56", "1_000.000", "1.000_000", "1_234.567_89", "1_234_567", "1_234e2", "1.234e2"}},
    {{"0_123", "0,123", "1_234,567", "1_000", "1,000", "1_234,56", "1,234_56", "1_000,000", "1,000_000", "1_234,567_89", "1_234_567", "1_234e2", "1,234e2"}},
    {{"0,123", "0.123", "1,234.567", "1,000", "1.000", "1,234.56", "1.23456", "1,000.000", "1.000000", "1,234.56789", "12,34,567", "1,234e2", "1.234e2"}}
}};

struct Input {
    const char* text;
    Literal dot;
    Literal comma;
    bool multiply = false;
};

inline constexpr std::array<Input, 17> inputs = {{
    {"0,123", LeadingZeroInteger, LeadingZeroFraction},
    {"0.123", LeadingZeroFraction, LeadingZeroInteger},
    {"1234,567", LongLeadingGroup, LongLeadingGroup},
    {"1234.567", LongLeadingGroup, LongLeadingGroup},
    {"1,000*2", Thousand, OneFraction, true},
    {"1.000*2", OneFraction, Thousand, true},
    {"1 000*2", Thousand, Thousand, true},
    {"1,234.56", ThousandFraction, ThousandFraction},
    {"1.234,56", ThousandFraction, ThousandFraction},
    {"1.000,000*2", ThousandZeroFraction, ThousandZeroFraction, true},
    {"1,000.000*2", ThousandZeroFraction, ThousandZeroFraction, true},
    {"1 234.567 89", LongFraction, LongFraction},
    {"1 234,567 89", LongFraction, LongFraction},
    {"12,34,567", Million, Million},
    {"12.34.567", Million, Million},
    {"1,234e2", ScientificInteger, ScientificFraction},
    {"1.234e2", ScientificFraction, ScientificInteger}
}};

inline constexpr std::array<const char*, 15> trailingZeroDisplays = {{
    "6.7980", "6,7980", "6.7980", "6,7980", "6.7980", "6,7980",
    "6.798 0", "6,798 0", "6.798,0", "6,798.0", "6.7980", "6,7980",
    "6.798_0", "6,798_0", "6.7980"
}};

struct Case {
    QString input;
    QString interpreted;
    QString displayed;
};

inline std::vector<Case> forFormat(size_t index)
{
    const auto& format = NumberFormatCases::formats.at(index);
    std::vector<Case> result;
    for (const auto& input : inputs) {
        Literal literal = format.commaDecimal ? input.comma : input.dot;
        if (format.style == Settings::NumberFormatThreeDigitCommaDotFraction) {
            if (QString::fromLatin1(input.text) == QLatin1String("1.234,56"))
                literal = SmallFraction;
            if (QString::fromLatin1(input.text) == QLatin1String("1.000,000*2"))
                literal = SmallZeroFraction;
        }
        if (format.style == Settings::NumberFormatThreeDigitDotCommaFraction) {
            if (QString::fromLatin1(input.text) == QLatin1String("1,234.56"))
                literal = SmallFraction;
            if (QString::fromLatin1(input.text) == QLatin1String("1,000.000*2"))
                literal = SmallZeroFraction;
        }
        result.push_back({QString::fromLatin1(input.text),
                          QString::fromLatin1(literals.at(literal)) + (input.multiply ? QString::fromUtf8("×2") : QString()),
                          QString::fromUtf8(displays.at(index).at(literal)) + (input.multiply ? QString::fromUtf8(" × 2") : QString())});
    }
    for (size_t i = 0; i < DecimalInputCases::cases.size(); ++i) {
        const auto& input = DecimalInputCases::cases.at(i);
        // The interpreted expression retains the typed trailing zero.
        const QString interpreted = i == 9 ? QStringLiteral("6.7980") : QString::fromLatin1(input.value);
        const QString displayed = i == 9
            ? QString::fromUtf8(trailingZeroDisplays.at(index))
            : QString::fromUtf8(DecimalInputCases::formats.at(index).outputs.at(i));
        result.push_back({QString::fromLatin1(input.input), interpreted, displayed});
    }
    result.push_back({QString::fromUtf8(format.example), QStringLiteral("1234567.12345"), QString::fromUtf8(format.example)});
    return result;
}

} // namespace InterpretedNumberCases

#endif
