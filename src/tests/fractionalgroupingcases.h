// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef TESTS_FRACTIONALGROUPINGCASES_H
#define TESTS_FRACTIONALGROUPINGCASES_H

#include "core/settings.h"

#include <array>

namespace FractionalGroupingCases {

struct Case {
    Settings::NumberFormatStyle style;
    const char* input;
    const char* value;
    const char* output;
};

// Prefer an ungrouped fractional part unless the selected format explicitly
// groups fractional digits with the comma or dot used in the input.
inline constexpr std::array<Case, 30> cases = {{
    {Settings::NumberFormatNoGroupingDot, "1.000,000 × 2", "2000", "2000"},
    {Settings::NumberFormatNoGroupingDot, "1,000.000 × 2", "2000", "2000"},
    {Settings::NumberFormatNoGroupingComma, "1.000,000 × 2", "2000", "2000"},
    {Settings::NumberFormatNoGroupingComma, "1,000.000 × 2", "2000", "2000"},
    {Settings::NumberFormatThreeDigitCommaDot, "1.000,000 × 2", "2000", "2,000"},
    {Settings::NumberFormatThreeDigitCommaDot, "1,000.000 × 2", "2000", "2,000"},
    {Settings::NumberFormatThreeDigitDotComma, "1.000,000 × 2", "2000", "2.000"},
    {Settings::NumberFormatThreeDigitDotComma, "1,000.000 × 2", "2000", "2.000"},
    {Settings::NumberFormatThreeDigitSpaceDot, "1.000,000 × 2", "2000", "2 000"},
    {Settings::NumberFormatThreeDigitSpaceDot, "1,000.000 × 2", "2000", "2 000"},
    {Settings::NumberFormatThreeDigitSpaceComma, "1.000,000 × 2", "2000", "2 000"},
    {Settings::NumberFormatThreeDigitSpaceComma, "1,000.000 × 2", "2000", "2 000"},
    {Settings::NumberFormatSIDot, "1.000,000 × 2", "2000", "2 000"},
    {Settings::NumberFormatSIDot, "1,000.000 × 2", "2000", "2 000"},
    {Settings::NumberFormatSIComma, "1.000,000 × 2", "2000", "2 000"},
    {Settings::NumberFormatSIComma, "1,000.000 × 2", "2000", "2 000"},
    {Settings::NumberFormatThreeDigitCommaDotFraction, "1.000,000 × 2", "2", "2"},
    {Settings::NumberFormatThreeDigitCommaDotFraction, "1,000.000 × 2", "2000", "2,000"},
    {Settings::NumberFormatThreeDigitDotCommaFraction, "1.000,000 × 2", "2000", "2.000"},
    {Settings::NumberFormatThreeDigitDotCommaFraction, "1,000.000 × 2", "2", "2"},
    {Settings::NumberFormatThreeDigitUnderscoreDot, "1.000,000 × 2", "2000", "2_000"},
    {Settings::NumberFormatThreeDigitUnderscoreDot, "1,000.000 × 2", "2000", "2_000"},
    {Settings::NumberFormatThreeDigitUnderscoreComma, "1.000,000 × 2", "2000", "2_000"},
    {Settings::NumberFormatThreeDigitUnderscoreComma, "1,000.000 × 2", "2000", "2_000"},
    {Settings::NumberFormatThreeDigitUnderscoreDotFraction, "1.000,000 × 2", "2000", "2_000"},
    {Settings::NumberFormatThreeDigitUnderscoreDotFraction, "1,000.000 × 2", "2000", "2_000"},
    {Settings::NumberFormatThreeDigitUnderscoreCommaFraction, "1.000,000 × 2", "2000", "2_000"},
    {Settings::NumberFormatThreeDigitUnderscoreCommaFraction, "1,000.000 × 2", "2000", "2_000"},
    {Settings::NumberFormatIndianCommaDot, "1.000,000 × 2", "2000", "2,000"},
    {Settings::NumberFormatIndianCommaDot, "1,000.000 × 2", "2000", "2,000"}
}};

} // namespace FractionalGroupingCases

#endif

