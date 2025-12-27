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

// see https://github.com/Tencent/rapidjson/issues/1448
// including windows.h on purpose to provoke a compile time problem as get_object is a
// macro that gets defined when windows.h is included
#ifdef _WIN32
#include <windows.h>
#endif

#include <merak/json/document.h>

using namespace merak::json;

TEST(Platform, GetObject) {
    Document doc;
    doc.parse(" { \"object\" : { \"pi\": 3.1416} } ");
    EXPECT_TRUE(doc.is_object());
    EXPECT_TRUE(doc.has_member("object"));
    const Document::ValueType& o = doc["object"];
    EXPECT_TRUE(o.is_object());
    Value::ConstObject sub = o.get_object();
    EXPECT_TRUE(sub.has_member("pi"));
    Value::ConstObject sub2 = o.get_object();
    EXPECT_TRUE(sub2.has_member("pi"));
}
