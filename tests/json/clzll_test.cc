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
#include <merak/json/internal/clzll.h>

#ifdef __GNUC__
MERAK_JSON_DIAG_PUSH
#endif

using namespace merak::json::internal;

TEST(clzll, normal) {
    EXPECT_EQ(clzll(1), 63U);
    EXPECT_EQ(clzll(2), 62U);
    EXPECT_EQ(clzll(12), 60U);
    EXPECT_EQ(clzll(0x0000000080000001UL), 32U);
    EXPECT_EQ(clzll(0x8000000000000001UL), 0U);
}

#ifdef __GNUC__
MERAK_JSON_DIAG_POP
#endif
