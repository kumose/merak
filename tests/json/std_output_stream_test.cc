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
