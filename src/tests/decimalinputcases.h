// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef TESTS_DECIMALINPUTCASES_H
#define TESTS_DECIMALINPUTCASES_H

#include "core/settings.h"

#include <array>

namespace DecimalInputCases {
struct Case {
    const char* input;
    const char* value;
};

// Dot-decimal format without display grouping. A comma can be an alternate
// decimal point when its position does not match a thousands group.
inline constexpr std::array<Case, 10> cases = {{
    {"1,2", "1.2"},
    {"3.4", "3.4"},
    {"35,6", "35.6"},
    {"56.7", "56.7"},
    {"12.23", "12.23"},
    {"34.56", "34.56"},
    {"987,1", "987.1"},
    {"876.2", "876.2"},
    {"2,34567", "2.34567"},
    {"6.7980", "6.798"}
}};
struct Format {
    Settings::NumberFormatStyle style;
    const char* example;
    std::array<const char*, cases.size()> outputs;
};

// Numeric values stay the same in every format. These literal expectations
// also check decimal marks and grouping in the displayed result.
inline constexpr std::array<Format, 15> formats = {{
    {Settings::NumberFormatNoGroupingDot, "1234567.12345",
     {{"1.2", "3.4", "35.6", "56.7", "12.23", "34.56", "987.1", "876.2", "2.34567", "6.798"}}},
    {Settings::NumberFormatNoGroupingComma, "1234567,12345",
     {{"1,2", "3,4", "35,6", "56,7", "12,23", "34,56", "987,1", "876,2", "2,34567", "6,798"}}},
    {Settings::NumberFormatThreeDigitCommaDot, "1,234,567.12345",
     {{"1.2", "3.4", "35.6", "56.7", "12.23", "34.56", "987.1", "876.2", "2.34567", "6.798"}}},
    {Settings::NumberFormatThreeDigitDotComma, "1.234.567,12345",
     {{"1,2", "3,4", "35,6", "56,7", "12,23", "34,56", "987,1", "876,2", "2,34567", "6,798"}}},
    {Settings::NumberFormatThreeDigitSpaceDot, "1 234 567.12345",
     {{"1.2", "3.4", "35.6", "56.7", "12.23", "34.56", "987.1", "876.2", "2.34567", "6.798"}}},
    {Settings::NumberFormatThreeDigitSpaceComma, "1 234 567,12345",
     {{"1,2", "3,4", "35,6", "56,7", "12,23", "34,56", "987,1", "876,2", "2,34567", "6,798"}}},
    {Settings::NumberFormatSIDot, "1 234 567.123 45",
     {{"1.2", "3.4", "35.6", "56.7", "12.23", "34.56", "987.1", "876.2", "2.345 67", "6.798"}}},
    {Settings::NumberFormatSIComma, "1 234 567,123 45",
     {{"1,2", "3,4", "35,6", "56,7", "12,23", "34,56", "987,1", "876,2", "2,345 67", "6,798"}}},
    {Settings::NumberFormatThreeDigitCommaDotFraction, "1,234,567.123,45",
     {{"1.2", "3.4", "35.6", "56.7", "12.23", "34.56", "987.1", "876.2", "2.345,67", "6.798"}}},
    {Settings::NumberFormatThreeDigitDotCommaFraction, "1.234.567,123.45",
     {{"1,2", "3,4", "35,6", "56,7", "12,23", "34,56", "987,1", "876,2", "2,345.67", "6,798"}}},
    {Settings::NumberFormatThreeDigitUnderscoreDot, "1_234_567.12345",
     {{"1.2", "3.4", "35.6", "56.7", "12.23", "34.56", "987.1", "876.2", "2.34567", "6.798"}}},
    {Settings::NumberFormatThreeDigitUnderscoreComma, "1_234_567,12345",
     {{"1,2", "3,4", "35,6", "56,7", "12,23", "34,56", "987,1", "876,2", "2,34567", "6,798"}}},
    {Settings::NumberFormatThreeDigitUnderscoreDotFraction, "1_234_567.123_45",
     {{"1.2", "3.4", "35.6", "56.7", "12.23", "34.56", "987.1", "876.2", "2.345_67", "6.798"}}},
    {Settings::NumberFormatThreeDigitUnderscoreCommaFraction, "1_234_567,123_45",
     {{"1,2", "3,4", "35,6", "56,7", "12,23", "34,56", "987,1", "876,2", "2,345_67", "6,798"}}},
    {Settings::NumberFormatIndianCommaDot, "12,34,567.12345",
     {{"1.2", "3.4", "35.6", "56.7", "12.23", "34.56", "987.1", "876.2", "2.34567", "6.798"}}}
}};

} // namespace DecimalInputCases

#endif

