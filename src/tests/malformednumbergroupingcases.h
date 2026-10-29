// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef TESTS_MALFORMEDNUMBERGROUPINGCASES_H
#define TESTS_MALFORMEDNUMBERGROUPINGCASES_H

#include <array>

namespace MalformedNumberGroupingCases {

// Every input must fail regardless of the selected number format.
inline constexpr std::array<const char*, 16> decimal = {{
    "1,2,3", "1.2.3",
    "1,,234", "1..234",
    "1,,234e2", "1..234e2", "1.2.3e2", "1,2,3e2",
    "0d1,,234", "0d1..234",
    "1,,234:00", "1..234:00",
    "1.123123,123123,32",
    "1,234,566.564,454545,45.23",
    "1.23.23,42342 242.4",
    "12,3.23,42342 242.4"
}};

// Binary and hexadecimal groups use four digits, octal groups use three.
// Repeated marks distinguish grouping errors from alternate decimals.
inline constexpr std::array<const char*, 12> nondecimal = {{
    "0b1,010,1010", "0b1.010.1010", "0b1,,1010", "0b1..1010",
    "0o1,23,456", "0o1.23.456", "0o1,,456", "0o1..456",
    "0x1,abc,def0", "0x1.abc.def0", "0x1,,def0", "0x1..def0"
}};

} // namespace MalformedNumberGroupingCases

#endif
