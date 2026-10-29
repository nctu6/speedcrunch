// SPDX-FileCopyrightText: 2013, 2015-2016, 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later


#ifndef CORE_NUMBERFORMATTER_H
#define CORE_NUMBERFORMATTER_H

#include "quantity.h"

#include <QtCore/QString>
#include <QtCore/QList>

struct NumberFormatter {
    static QString format(HNumber &num) { return format(Quantity(num)); }
    static QString format(CNumber &num) { return format(Quantity(num)); }
    static QString format(Quantity);
    static QString format(Quantity, char resultFormatOverride);
    static QString format(Quantity, char resultFormatOverride, int precisionOverride,
                          bool useComplexNotation, char complexNotationOverride);
    static QString formatTrigSymbolic(Quantity);
    static QString formatNumericLiteralForDisplay(const QString& input);
    static bool isSingleGroupedInteger(const QString& input);
    static bool hasFormattedFractionalGrouping(const QString& input);
    static bool hasValidNumberGrouping(const QString& input, QChar radix, int base = 10);
    static QList<int> digitGroupingPositions(int digitCount, int groupSize, bool fractional);
    static QString rewriteScientificNotationForDisplay(const QString& input);
    static bool tryFormatStandaloneNumericLiteralForDisplay(const QString& input, QString* output);
};

#endif
