// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef TESTS_NUMBERGROUPINGEDGECASES_H
#define TESTS_NUMBERGROUPINGEDGECASES_H

#include <array>

namespace NumberGroupingEdgeCases {
struct Case {
    const char* input;
    const char* value; // A null value expects an evaluation error.
};

inline constexpr std::array<Case, 18> dotCases = {{
    {"0,123", "123"}, {"1234,567", "1234.567"},
    {"1,2,3", nullptr}, {"1.2.3", nullptr}, {"1,,234", nullptr}, {"12 34", "1234"},
    {"1,,234e2", nullptr}, {"1.2.3e2", nullptr}, {"0d1,,234", nullptr},
    {"1,234,567", "1234567"}, {"1.234.567", "1234567"},
    {"12,34,567", "1234567"}, {"12.34.567", "1234567"},
    {"1,234,567.89", "1234567.89"}, {"1.234.567,89", "1234567.89"},
    {"1,234.567,89", "1234.56789"}, {"1.234,567.89", "1234.56789"},
    {"1 2 3", "123"}
}};

inline constexpr std::array<Case, 18> commaCases = {{
    {"0.123", "123"}, {"1234.567", "1234.567"},
    {"1.2.3", nullptr}, {"1,2,3", nullptr}, {"1..234", nullptr}, {"12 34", "1234"},
    {"1..234e2", nullptr}, {"1,2,3e2", nullptr}, {"0d1..234", nullptr},
    {"1.234.567", "1234567"}, {"1,234,567", "1234567"},
    {"12.34.567", "1234567"}, {"12,34,567", "1234567"},
    {"1.234.567,89", "1234567.89"}, {"1,234,567.89", "1234567.89"},
    {"1.234,567.89", "1234.56789"}, {"1,234.567,89", "1234.56789"},
    {"1 2 3", "123"}
}};
} // namespace NumberGroupingEdgeCases

#endif

