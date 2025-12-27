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
#include <merak/json/document.h>
#include <merak/json/cursorstreamwrapper.h>

using namespace merak::json;

// static const char json[] = "{\"string\"\n\n:\"my string\",\"array\"\n:[\"1\", \"2\", \"3\"]}";

static bool testJson(const char *json, size_t &line, size_t &col) {
    StringStream ss(json);
    CursorStreamWrapper<StringStream> csw(ss);
    Document document;
    document.parse_stream(csw);
    bool ret = document.has_parse_error();
    if (ret) {
        col = csw.GetColumn();
        line = csw.GetLine();
    }
    return ret;
}

TEST(CursorStreamWrapper, MissingFirstBracket) {
    const char json[] = "\"string\"\n\n:\"my string\",\"array\"\n:[\"1\", \"2\", \"3\"]}";
    size_t col, line;
    bool ret = testJson(json, line, col);
    EXPECT_TRUE(ret);
    EXPECT_EQ(line, 3u);
    EXPECT_EQ(col, 0u);
}

TEST(CursorStreamWrapper, MissingQuotes) {
    const char json[] = "{\"string\n\n:\"my string\",\"array\"\n:[\"1\", \"2\", \"3\"]}";
    size_t col, line;
    bool ret = testJson(json, line, col);
    EXPECT_TRUE(ret);
    EXPECT_EQ(line, 1u);
    EXPECT_EQ(col, 8u);
}

TEST(CursorStreamWrapper, MissingColon) {
    const char json[] = "{\"string\"\n\n\"my string\",\"array\"\n:[\"1\", \"2\", \"3\"]}";
    size_t col, line;
    bool ret = testJson(json, line, col);
    EXPECT_TRUE(ret);
    EXPECT_EQ(line, 3u);
    EXPECT_EQ(col, 0u);
}

TEST(CursorStreamWrapper, MissingSecondQuotes) {
    const char json[] = "{\"string\"\n\n:my string\",\"array\"\n:[\"1\", \"2\", \"3\"]}";
    size_t col, line;
    bool ret = testJson(json, line, col);
    EXPECT_TRUE(ret);
    EXPECT_EQ(line, 3u);
    EXPECT_EQ(col, 1u);
}

TEST(CursorStreamWrapper, MissingComma) {
    const char json[] = "{\"string\"\n\n:\"my string\"\"array\"\n:[\"1\", \"2\", \"3\"]}";
    size_t col, line;
    bool ret = testJson(json, line, col);
    EXPECT_TRUE(ret);
    EXPECT_EQ(line, 3u);
    EXPECT_EQ(col, 12u);
}

TEST(CursorStreamWrapper, MissingArrayBracket) {
    const char json[] = "{\"string\"\n\n:\"my string\",\"array\"\n:\"1\", \"2\", \"3\"]}";
    size_t col, line;
    bool ret = testJson(json, line, col);
    EXPECT_TRUE(ret);
    EXPECT_EQ(line, 4u);
    EXPECT_EQ(col, 9u);
}

TEST(CursorStreamWrapper, MissingArrayComma) {
    const char json[] = "{\"string\"\n\n:\"my string\",\"array\"\n:[\"1\" \"2\", \"3\"]}";
    size_t col, line;
    bool ret = testJson(json, line, col);
    EXPECT_TRUE(ret);
    EXPECT_EQ(line, 4u);
    EXPECT_EQ(col, 6u);
}

TEST(CursorStreamWrapper, MissingLastArrayBracket) {
    const char json8[] = "{\"string\"\n\n:\"my string\",\"array\"\n:[\"1\", \"2\", \"3\"}";
    size_t col, line;
    bool ret = testJson(json8, line, col);
    EXPECT_TRUE(ret);
    EXPECT_EQ(line, 4u);
    EXPECT_EQ(col, 15u);
}

TEST(CursorStreamWrapper, MissingLastBracket) {
    const char json9[] = "{\"string\"\n\n:\"my string\",\"array\"\n:[\"1\", \"2\", \"3\"]";
    size_t col, line;
    bool ret = testJson(json9, line, col);
    EXPECT_TRUE(ret);
    EXPECT_EQ(line, 4u);
    EXPECT_EQ(col, 16u);
}
