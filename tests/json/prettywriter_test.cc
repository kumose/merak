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
#include <merak/json/reader.h>
#include <merak/json/prettywriter.h>
#include <merak/json/std_output_stream.h>
#include <merak/json/stringbuffer.h>
#include <merak/json/filewritestream.h>
#include <sstream>

#ifdef __clang__
MERAK_JSON_DIAG_PUSH
MERAK_JSON_DIAG_OFF(c++98-compat)
#endif

using namespace merak::json;

static const char kJson[] = "{\"hello\":\"world\",\"t\":true,\"f\":false,\"n\":null,\"i\":123,\"pi\":3.1416,\"a\":[1,2,3,-1],\"u64\":1234567890123456789,\"i64\":-1234567890123456789}";
static const char kPrettyJson[] =
"{\n"
"    \"hello\": \"world\",\n"
"    \"t\": true,\n"
"    \"f\": false,\n"
"    \"n\": null,\n"
"    \"i\": 123,\n"
"    \"pi\": 3.1416,\n"
"    \"a\": [\n"
"        1,\n"
"        2,\n"
"        3,\n"
"        -1\n"
"    ],\n"
"    \"u64\": 1234567890123456789,\n"
"    \"i64\": -1234567890123456789\n"
"}";

static const char kPrettyJson_FormatOptions_SLA[] =
"{\n"
"    \"hello\": \"world\",\n"
"    \"t\": true,\n"
"    \"f\": false,\n"
"    \"n\": null,\n"
"    \"i\": 123,\n"
"    \"pi\": 3.1416,\n"
"    \"a\": [1, 2, 3, -1],\n"
"    \"u64\": 1234567890123456789,\n"
"    \"i64\": -1234567890123456789\n"
"}";

TEST(PrettyWriter, Basic) {
    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    Reader reader;
    StringStream s(kJson);
    reader.parse(s, writer);
    EXPECT_STREQ(kPrettyJson, buffer.get_string());
}

TEST(PrettyWriter, FormatOptions) {
    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    writer.SetFormatOptions(kFormatSingleLineArray);
    Reader reader;
    StringStream s(kJson);
    reader.parse(s, writer);
    EXPECT_STREQ(kPrettyJson_FormatOptions_SLA, buffer.get_string());
}

TEST(PrettyWriter, SetIndent) {
    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    writer.SetIndent('\t', 1);
    Reader reader;
    StringStream s(kJson);
    reader.parse(s, writer);
    EXPECT_STREQ(
        "{\n"
        "\t\"hello\": \"world\",\n"
        "\t\"t\": true,\n"
        "\t\"f\": false,\n"
        "\t\"n\": null,\n"
        "\t\"i\": 123,\n"
        "\t\"pi\": 3.1416,\n"
        "\t\"a\": [\n"
        "\t\t1,\n"
        "\t\t2,\n"
        "\t\t3,\n"
        "\t\t-1\n"
        "\t],\n"
        "\t\"u64\": 1234567890123456789,\n"
        "\t\"i64\": -1234567890123456789\n"
        "}",
        buffer.get_string());
}

TEST(PrettyWriter, String) {
    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    EXPECT_TRUE(writer.start_array());
    EXPECT_TRUE(writer.emplace_string("Hello\n"));
    EXPECT_TRUE(writer.end_array());
    EXPECT_STREQ("[\n    \"Hello\\n\"\n]", buffer.get_string());
}


TEST(PrettyWriter, String_STDSTRING) {
    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    EXPECT_TRUE(writer.start_array());
    EXPECT_TRUE(writer.emplace_string(std::string("Hello\n")));
    EXPECT_TRUE(writer.end_array());
    EXPECT_STREQ("[\n    \"Hello\\n\"\n]", buffer.get_string());
}

// For covering PutN() generic version
TEST(PrettyWriter, StdOutputStream) {
    StringStream s(kJson);
    
    std::stringstream ss;
    StdOutputStream os(ss);
    
    PrettyWriter<StdOutputStream> writer(os);

    Reader reader;
    reader.parse(s, writer);
    
    std::string actual = ss.str();
    EXPECT_STREQ(kPrettyJson, actual.c_str());
}

// For covering FileWriteStream::PutN()
TEST(PrettyWriter, FileWriteStream) {
    char filename[L_tmpnam];
    FILE* fp = TempFile(filename);
    ASSERT_TRUE(fp!=NULL);
    char buffer[16];
    FileWriteStream os(fp, buffer, sizeof(buffer));
    PrettyWriter<FileWriteStream> writer(os);
    Reader reader;
    StringStream s(kJson);
    reader.parse(s, writer);
    fclose(fp);

    fp = fopen(filename, "rb");
    fseek(fp, 0, SEEK_END);
    size_t size = static_cast<size_t>(ftell(fp));
    fseek(fp, 0, SEEK_SET);
    char* json = static_cast<char*>(malloc(size + 1));
    size_t readLength = fread(json, 1, size, fp);
    json[readLength] = '\0';
    fclose(fp);
    remove(filename);
    EXPECT_STREQ(kPrettyJson, json);
    free(json);
}

TEST(PrettyWriter, raw_value) {
    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    writer.start_object();
    writer.emplace_key("a");
    writer.emplace_int32(1);
    writer.emplace_key("raw");
    const char json[] = "[\"Hello\\nWorld\", 123.456]";
    writer.raw_value(json, strlen(json), kArrayType);
    writer.end_object();
    EXPECT_TRUE(writer.is_complete());
    EXPECT_STREQ(
        "{\n"
        "    \"a\": 1,\n"
        "    \"raw\": [\"Hello\\nWorld\", 123.456]\n" // no indentation within raw value
        "}",
        buffer.get_string());
}

TEST(PrettyWriter, InvalidEventSequence) {
    // {]
    {
        StringBuffer buffer;
        PrettyWriter<StringBuffer> writer(buffer);
        writer.start_object();
        EXPECT_THROW(writer.end_array(), AssertException);
        EXPECT_FALSE(writer.is_complete());
    }
    
    // [}
    {
        StringBuffer buffer;
        PrettyWriter<StringBuffer> writer(buffer);
        writer.start_array();
        EXPECT_THROW(writer.end_object(), AssertException);
        EXPECT_FALSE(writer.is_complete());
    }
    
    // { 1:
    {
        StringBuffer buffer;
        PrettyWriter<StringBuffer> writer(buffer);
        writer.start_object();
        EXPECT_THROW(writer.emplace_int32(1), AssertException);
        EXPECT_FALSE(writer.is_complete());
    }
    
    // { 'a' }
    {
        StringBuffer buffer;
        PrettyWriter<StringBuffer> writer(buffer);
        writer.start_object();
        writer.emplace_key("a");
        EXPECT_THROW(writer.end_object(), AssertException);
        EXPECT_FALSE(writer.is_complete());
    }
    
    // { 'a':'b','c' }
    {
        StringBuffer buffer;
        PrettyWriter<StringBuffer> writer(buffer);
        writer.start_object();
        writer.emplace_key("a");
        writer.emplace_string("b");
        writer.emplace_key("c");
        EXPECT_THROW(writer.end_object(), AssertException);
        EXPECT_FALSE(writer.is_complete());
    }
}

TEST(PrettyWriter, NaN) {
    double nan = std::numeric_limits<double>::quiet_NaN();

    EXPECT_TRUE(internal::Double(nan).IsNan());
    StringBuffer buffer;
    {
        PrettyWriter<StringBuffer> writer(buffer);
        EXPECT_FALSE(writer.emplace_double(nan));
    }
    {
        PrettyWriter<StringBuffer, UTF8<>, UTF8<>, CrtAllocator, kWriteNanAndInfFlag> writer(buffer);
        EXPECT_TRUE(writer.emplace_double(nan));
        EXPECT_STREQ("NaN", buffer.get_string());
    }
    GenericStringBuffer<UTF16<> > buffer2;
    PrettyWriter<GenericStringBuffer<UTF16<> > > writer2(buffer2);
    EXPECT_FALSE(writer2.emplace_double(nan));
}

TEST(PrettyWriter, Inf) {
    double inf = std::numeric_limits<double>::infinity();

    EXPECT_TRUE(internal::Double(inf).IsInf());
    StringBuffer buffer;
    {
        PrettyWriter<StringBuffer> writer(buffer);
        EXPECT_FALSE(writer.emplace_double(inf));
    }
    {
        PrettyWriter<StringBuffer> writer(buffer);
        EXPECT_FALSE(writer.emplace_double(-inf));
    }
    {
        PrettyWriter<StringBuffer, UTF8<>, UTF8<>, CrtAllocator, kWriteNanAndInfFlag> writer(buffer);
        EXPECT_TRUE(writer.emplace_double(inf));
    }
    {
        PrettyWriter<StringBuffer, UTF8<>, UTF8<>, CrtAllocator, kWriteNanAndInfFlag> writer(buffer);
        EXPECT_TRUE(writer.emplace_double(-inf));
    }
    EXPECT_STREQ("Infinity-Infinity", buffer.get_string());
}

TEST(PrettyWriter, Issue_889) {
    char buf[100] = "Hello";
    
    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    writer.start_array();
    writer.emplace_string(buf);
    writer.end_array();
    
    EXPECT_STREQ("[\n    \"Hello\"\n]", buffer.get_string());
    EXPECT_TRUE(writer.is_complete()); \
}



static PrettyWriter<StringBuffer> WriterGen(StringBuffer &target) {
    PrettyWriter<StringBuffer> writer(target);
    writer.start_object();
    writer.emplace_key("a");
    writer.emplace_int32(1);
    return writer;
}

TEST(PrettyWriter, MoveCtor) {
    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(WriterGen(buffer));
    writer.end_object();
    EXPECT_TRUE(writer.is_complete());
    EXPECT_STREQ(
        "{\n"
        "    \"a\": 1\n"
        "}",
        buffer.get_string());
}


TEST(PrettyWriter, Issue_1336) {
#define T(meth, val, expected)                          \
    {                                                   \
        StringBuffer buffer;                            \
        PrettyWriter<StringBuffer> writer(buffer);      \
        writer.meth(val);                               \
                                                        \
        EXPECT_STREQ(expected, buffer.get_string());     \
        EXPECT_TRUE(writer.is_complete());               \
    }

    T(emplace_bool, false, "false");
    T(emplace_bool, true, "true");
    T(emplace_int32, 0, "0");
    T(emplace_uint32, 0, "0");
    T(emplace_int64, 0, "0");
    T(emplace_uint64, 0, "0");
    T(emplace_double, 0, "0.0");
    T(emplace_string, "Hello", "\"Hello\"");
#undef T

    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    writer.emplace_null();

    EXPECT_STREQ("null", buffer.get_string());
    EXPECT_TRUE(writer.is_complete());
}

#ifdef __clang__
MERAK_JSON_DIAG_POP
#endif
