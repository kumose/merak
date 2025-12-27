// Copyright (C) 2024 Kumo inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
// Copyright (C) 2015 THL A29 Limited, a Tencent company, and Milo Yip.

#include <tests/json/unittest.h>
#include <merak/json/internal/dtoa.h>

#ifdef __GNUC__
MERAK_JSON_DIAG_PUSH
MERAK_JSON_DIAG_OFF(type-limits)
#endif

using namespace merak::json::internal;

TEST(dtoa, normal) {
    char buffer[30];

#define TEST_DTOA(d, a)\
    *dtoa(d, buffer) = '\0';\
    EXPECT_STREQ(a, buffer)

    TEST_DTOA(0.0, "0.0");
    TEST_DTOA(-0.0, "-0.0");
    TEST_DTOA(1.0, "1.0");
    TEST_DTOA(-1.0, "-1.0");
    TEST_DTOA(1.2345, "1.2345");
    TEST_DTOA(1.2345678, "1.2345678");
    TEST_DTOA(0.123456789012, "0.123456789012");
    TEST_DTOA(1234567.8, "1234567.8");
    TEST_DTOA(-79.39773355813419, "-79.39773355813419");
    TEST_DTOA(-36.973846435546875, "-36.973846435546875");
    TEST_DTOA(0.000001, "0.000001");
    TEST_DTOA(0.0000001, "1e-7");
    TEST_DTOA(1e30, "1e30");
    TEST_DTOA(1.234567890123456e30, "1.234567890123456e30");
    TEST_DTOA(5e-324, "5e-324"); // Min subnormal positive double
    TEST_DTOA(2.225073858507201e-308, "2.225073858507201e-308"); // Max subnormal positive double
    TEST_DTOA(2.2250738585072014e-308, "2.2250738585072014e-308"); // Min normal positive double
    TEST_DTOA(1.7976931348623157e308, "1.7976931348623157e308"); // Max double

#undef TEST_DTOA
}

TEST(dtoa, maxDecimalPlaces) {
    char buffer[30];

#define TEST_DTOA(m, d, a)\
    *dtoa(d, buffer, m) = '\0';\
    EXPECT_STREQ(a, buffer)

    TEST_DTOA(3, 0.0, "0.0");
    TEST_DTOA(1, 0.0, "0.0");
    TEST_DTOA(3, -0.0, "-0.0");
    TEST_DTOA(3, 1.0, "1.0");
    TEST_DTOA(3, -1.0, "-1.0");
    TEST_DTOA(3, 1.2345, "1.234");
    TEST_DTOA(2, 1.2345, "1.23");
    TEST_DTOA(1, 1.2345, "1.2");
    TEST_DTOA(3, 1.2345678, "1.234");
    TEST_DTOA(3, 1.0001, "1.0");
    TEST_DTOA(2, 1.0001, "1.0");
    TEST_DTOA(1, 1.0001, "1.0");
    TEST_DTOA(3, 0.123456789012, "0.123");
    TEST_DTOA(2, 0.123456789012, "0.12");
    TEST_DTOA(1, 0.123456789012, "0.1");
    TEST_DTOA(4, 0.0001, "0.0001");
    TEST_DTOA(3, 0.0001, "0.0");
    TEST_DTOA(2, 0.0001, "0.0");
    TEST_DTOA(1, 0.0001, "0.0");
    TEST_DTOA(3, 1234567.8, "1234567.8");
    TEST_DTOA(3, 1e30, "1e30");
    TEST_DTOA(3, 5e-324, "0.0"); // Min subnormal positive double
    TEST_DTOA(3, 2.225073858507201e-308, "0.0"); // Max subnormal positive double
    TEST_DTOA(3, 2.2250738585072014e-308, "0.0"); // Min normal positive double
    TEST_DTOA(3, 1.7976931348623157e308, "1.7976931348623157e308"); // Max double
    TEST_DTOA(5, -0.14000000000000001, "-0.14");
    TEST_DTOA(4, -0.14000000000000001, "-0.14");
    TEST_DTOA(3, -0.14000000000000001, "-0.14");
    TEST_DTOA(3, -0.10000000000000001, "-0.1");
    TEST_DTOA(2, -0.10000000000000001, "-0.1");
    TEST_DTOA(1, -0.10000000000000001, "-0.1");

#undef TEST_DTOA
}


#ifdef __GNUC__
MERAK_JSON_DIAG_POP
#endif
