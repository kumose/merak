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

#include <merak/json/std_input_stream.h>
#include <merak/json/encodedstream.h>
#include <merak/json/document.h>
#include <sstream>
#include <fstream>

#if defined(_MSC_VER) && !defined(__clang__)
MERAK_JSON_DIAG_PUSH
MERAK_JSON_DIAG_OFF(4702) // unreachable code
#endif

using namespace merak::json;
using namespace std;


template <typename FileStreamType>
static bool Open(FileStreamType& fs, const char* filename) {
    const char *paths[] = {
        "encodings",
        "tests/bin/encodings",
        "../tests/bin/encodings",
        "../../tests/bin/encodings",
        "../../../tests/bin/encodings"
    };
    char buffer[1024];
    for (size_t i = 0; i < sizeof(paths) / sizeof(paths[0]); i++) {
        sprintf(buffer, "%s/%s", paths[i], filename);
        fs.open(buffer, ios_base::in | ios_base::binary);
        if (fs.is_open())
            return true;
    }
    return false;
}

TEST(FileInputStream, ifstream) {
    ifstream ifs;
    ASSERT_TRUE(Open(ifs, "utf8bom.json"));
    FileInputStream isw(ifs);
    EncodedInputStream<UTF8<>, FileInputStream> eis(isw);
    Document d;
    EXPECT_TRUE(!d.parse_stream(eis).has_parse_error());
    EXPECT_TRUE(d.is_object());
    EXPECT_EQ(5u, d.member_count());
}

TEST(FileInputStream, fstream) {
    fstream fs;
    ASSERT_TRUE(Open(fs, "utf8bom.json"));
    FileInputStream isw(fs);
    EncodedInputStream<UTF8<>, FileInputStream> eis(isw);
    Document d;
    EXPECT_TRUE(!d.parse_stream(eis).has_parse_error());
    EXPECT_TRUE(d.is_object());
    EXPECT_EQ(5u, d.member_count());
}

// wifstream/wfstream only works on C++11 with codecvt_utf16
// But many C++11 library still not have it.
#if 0
#include <codecvt>

TEST(FileInputStream, wifstream) {
    wifstream ifs;
    ASSERT_TRUE(Open(ifs, "utf16bebom.json"));
    ifs.imbue(std::locale(ifs.getloc(),
       new std::codecvt_utf16<wchar_t, 0x10ffff, std::consume_header>));
    WFileInputStream isw(ifs);
    GenericDocument<UTF16<> > d;
    d.parse_stream<kParseDefaultFlags, UTF16<>, WFileInputStream>(isw);
    EXPECT_TRUE(!d.has_parse_error());
    EXPECT_TRUE(d.is_object());
    EXPECT_EQ(5, d.member_count());
}

TEST(FileInputStream, wfstream) {
    wfstream fs;
    ASSERT_TRUE(Open(fs, "utf16bebom.json"));
    fs.imbue(std::locale(fs.getloc(),
       new std::codecvt_utf16<wchar_t, 0x10ffff, std::consume_header>));
    WFileInputStream isw(fs);
    GenericDocument<UTF16<> > d;
    d.parse_stream<kParseDefaultFlags, UTF16<>, WFileInputStream>(isw);
    EXPECT_TRUE(!d.has_parse_error());
    EXPECT_TRUE(d.is_object());
    EXPECT_EQ(5, d.member_count());
}

#endif

#if defined(_MSC_VER) && !defined(__clang__)
MERAK_JSON_DIAG_POP
#endif
