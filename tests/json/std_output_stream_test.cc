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

#include <merak/json/std_output_stream.h>
#include <merak/json/encodedstream.h>
#include <merak/json/document.h>
#include <sstream>
#include <fstream>

using namespace merak::json;
using namespace std;

template <typename StringStreamType>
static void TestStringStream() {
    typedef typename StringStreamType::char_type char_type;

    char_type s[] = { 'A', 'B', 'C', '\0' };
    StringStreamType oss(s);
    BasicStdOutputStream<StringStreamType> os(oss);
    for (size_t i = 0; i < 3; i++)
        os.put(s[i]);
    os.flush();
    for (size_t i = 0; i < 3; i++)
        EXPECT_EQ(s[i], oss.str()[i]);
}

TEST(StdOutputStream, ostringstream) {
    TestStringStream<ostringstream>();
}

TEST(StdOutputStream, stringstream) {
    TestStringStream<stringstream>();
}

TEST(StdOutputStream, wostringstream) {
    TestStringStream<wostringstream>();
}

TEST(StdOutputStream, wstringstream) {
    TestStringStream<wstringstream>();
}

TEST(StdOutputStream, cout) {
    StdOutputStream os(cout);
    const char* s = "Hello World!\n";
    while (*s)
        os.put(*s++);
    os.flush();
}

template <typename FileStreamType>
static void TestFileStream() {
    char filename[L_tmpnam];
    FILE* fp = TempFile(filename);
    fclose(fp);

    const char* s = "Hello World!\n";
    {
        FileStreamType ofs(filename, ios::out | ios::binary);
        BasicStdOutputStream<FileStreamType> osw(ofs);
        for (const char* p = s; *p; p++)
            osw.put(*p);
        osw.flush();
    }

    fp = fopen(filename, "r");
    ASSERT_TRUE( fp != NULL );
    for (const char* p = s; *p; p++)
        EXPECT_EQ(*p, static_cast<char>(fgetc(fp)));
    fclose(fp);
}

TEST(StdOutputStream, ofstream) {
    TestFileStream<ofstream>();
}

TEST(StdOutputStream, fstream) {
    TestFileStream<fstream>();
}
