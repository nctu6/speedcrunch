// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef TESTS_NUMBERFORMATCASES_H
#define TESTS_NUMBERFORMATCASES_H

#include "core/settings.h"

#include <array>

namespace NumberFormatCases {

// Keep these expectations literal so parsing and formatting cannot define
// their own expected results. The rows follow the Number Format dialog.
inline constexpr std::array<const char*, 11> inputs = {
    "1,000 × 2",
    "1.000 × 2",
    "1 000 × 2",
    "1,23",
    "1.23",
    "1,234.56",
    "1.234,56",
    "1 234.567 89",
    "1 234,567 89",
    "1,234e2",
    "1.234e2"
};

struct Format {
    Settings::NumberFormatStyle style;
    const char* example;
    bool commaDecimal;
    std::array<const char*, inputs.size()> outputs;
};

inline constexpr std::array<Format, 15> formats = {{
    {Settings::NumberFormatNoGroupingDot, "1234567.12345", false,
     {{"2000", "2", "2000", "1.23", "1.23", "1234.56", "1234.56", "1234.56789", "1234.56789", "123400", "123.4"}}},
    {Settings::NumberFormatNoGroupingComma, "1234567,12345", true,
     {{"2", "2000", "2000", "1,23", "1,23", "1234,56", "1234,56", "1234,56789", "1234,56789", "123,4", "123400"}}},
    {Settings::NumberFormatThreeDigitCommaDot, "1,234,567.12345", false,
     {{"2,000", "2", "2,000", "1.23", "1.23", "1,234.56", "1,234.56", "1,234.56789", "1,234.56789", "123,400", "123.4"}}},
    {Settings::NumberFormatThreeDigitDotComma, "1.234.567,12345", true,
     {{"2", "2.000", "2.000", "1,23", "1,23", "1.234,56", "1.234,56", "1.234,56789", "1.234,56789", "123,4", "123.400"}}},
    {Settings::NumberFormatThreeDigitSpaceDot, "1 234 567.12345", false,
     {{"2 000", "2", "2 000", "1.23", "1.23", "1 234.56", "1 234.56", "1 234.56789", "1 234.56789", "123 400", "123.4"}}},
    {Settings::NumberFormatThreeDigitSpaceComma, "1 234 567,12345", true,
     {{"2", "2 000", "2 000", "1,23", "1,23", "1 234,56", "1 234,56", "1 234,56789", "1 234,56789", "123,4", "123 400"}}},
    {Settings::NumberFormatSIDot, "1 234 567.123 45", false,
     {{"2 000", "2", "2 000", "1.23", "1.23", "1 234.56", "1 234.56", "1 234.567 89", "1 234.567 89", "123 400", "123.4"}}},
    {Settings::NumberFormatSIComma, "1 234 567,123 45", true,
     {{"2", "2 000", "2 000", "1,23", "1,23", "1 234,56", "1 234,56", "1 234,567 89", "1 234,567 89", "123,4", "123 400"}}},
    {Settings::NumberFormatThreeDigitCommaDotFraction, "1,234,567.123,45", false,
     {{"2,000", "2", "2,000", "1.23", "1.23", "1,234.56", "1.234,56", "1,234.567,89", "1,234.567,89", "123,400", "123.4"}}},
    {Settings::NumberFormatThreeDigitDotCommaFraction, "1.234.567,123.45", true,
     {{"2", "2.000", "2.000", "1,23", "1,23", "1,234.56", "1.234,56", "1.234,567.89", "1.234,567.89", "123,4", "123.400"}}},
    {Settings::NumberFormatThreeDigitUnderscoreDot, "1_234_567.12345", false,
     {{"2_000", "2", "2_000", "1.23", "1.23", "1_234.56", "1_234.56", "1_234.56789", "1_234.56789", "123_400", "123.4"}}},
    {Settings::NumberFormatThreeDigitUnderscoreComma, "1_234_567,12345", true,
     {{"2", "2_000", "2_000", "1,23", "1,23", "1_234,56", "1_234,56", "1_234,56789", "1_234,56789", "123,4", "123_400"}}},
    {Settings::NumberFormatThreeDigitUnderscoreDotFraction, "1_234_567.123_45", false,
     {{"2_000", "2", "2_000", "1.23", "1.23", "1_234.56", "1_234.56", "1_234.567_89", "1_234.567_89", "123_400", "123.4"}}},
    {Settings::NumberFormatThreeDigitUnderscoreCommaFraction, "1_234_567,123_45", true,
     {{"2", "2_000", "2_000", "1,23", "1,23", "1_234,56", "1_234,56", "1_234,567_89", "1_234,567_89", "123,4", "123_400"}}},
    {Settings::NumberFormatIndianCommaDot, "12,34,567.12345", false,
     {{"2,000", "2", "2,000", "1.23", "1.23", "1,234.56", "1,234.56", "1,234.56789", "1,234.56789", "1,23,400", "123.4"}}}
}};

inline const char* numericValue(const Format& format, size_t index)
{
    static constexpr std::array<const char*, inputs.size()> dotValues = {
        "2000", "2", "2000", "1.23", "1.23", "1234.56", "1234.56",
        "1234.56789", "1234.56789", "123400", "123.4"
    };
    static constexpr std::array<const char*, inputs.size()> commaValues = {
        "2", "2000", "2000", "1.23", "1.23", "1234.56", "1234.56",
        "1234.56789", "1234.56789", "123.4", "123400"
    };
    // The selected fractional-grouping convention wins these two ambiguities.
    if ((format.style == Settings::NumberFormatThreeDigitCommaDotFraction && index == 6)
        || (format.style == Settings::NumberFormatThreeDigitDotCommaFraction && index == 5))
        return "1.23456";
    return format.commaDecimal ? commaValues.at(index) : dotValues.at(index);
}

} // namespace NumberFormatCases

#endif

