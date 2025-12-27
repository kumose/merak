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
#include <merak/json/internal/strfunc.h>

using namespace merak::json;
using namespace merak::json::internal;

TEST(StrFunc, CountStringCodePoint) {
    SizeType count;
    EXPECT_TRUE(CountStringCodePoint<UTF8<> >("", 0, &count));
    EXPECT_EQ(0u, count);
    EXPECT_TRUE(CountStringCodePoint<UTF8<> >("Hello", 5, &count));
    EXPECT_EQ(5u, count);
    EXPECT_TRUE(CountStringCodePoint<UTF8<> >("\xC2\xA2\xE2\x82\xAC\xF0\x9D\x84\x9E", 9, &count)); // cents euro G-clef
    EXPECT_EQ(3u, count);
    EXPECT_FALSE(CountStringCodePoint<UTF8<> >("\xC2\xA2\xE2\x82\xAC\xF0\x9D\x84\x9E\x80", 10, &count));
}
