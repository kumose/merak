// Copyright (C) 2026 Kumo inc. and its affiliates. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
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
