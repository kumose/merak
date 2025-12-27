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
#include <merak/json/document.h>
#include <merak/json/reader.h>
#include <merak/json/writer.h>
#include <merak/json/stringbuffer.h>
#include <merak/json/memorybuffer.h>
#include <sstream>

#ifdef __clang__
MERAK_JSON_DIAG_PUSH
MERAK_JSON_DIAG_OFF(c++98-compat)
#endif

using namespace merak::json;

TEST(Writer, Compact) {
    StringStream s("{ \"hello\" : \"world\", \"t\" : true , \"f\" : false, \"n\": null, \"i\":123, \"pi\": 3.1416, \"a\":[1, 2, 3] } ");
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    buffer.ShrinkToFit();
    Reader reader;
    reader.parse<0>(s, writer);
    EXPECT_STREQ("{\"hello\":\"world\",\"t\":true,\"f\":false,\"n\":null,\"i\":123,\"pi\":3.1416,\"a\":[1,2,3]}", buffer.get_string());
    EXPECT_EQ(77u, buffer.GetSize());
    EXPECT_TRUE(writer.is_complete());
}

// json -> parse -> writer -> json
#define TEST_ROUNDTRIP(json) \
    { \
        StringStream s(json); \
        StringBuffer buffer; \
        Writer<StringBuffer> writer(buffer); \
        Reader reader; \
        reader.parse<kParseFullPrecisionFlag>(s, writer); \
        EXPECT_STREQ(json, buffer.get_string()); \
        EXPECT_TRUE(writer.is_complete()); \
    }

TEST(Writer, Root) {
    TEST_ROUNDTRIP("null");
    TEST_ROUNDTRIP("true");
    TEST_ROUNDTRIP("false");
    TEST_ROUNDTRIP("0");
    TEST_ROUNDTRIP("\"foo\"");
    TEST_ROUNDTRIP("[]");
    TEST_ROUNDTRIP("{}");
}

TEST(Writer, Int) {
    TEST_ROUNDTRIP("[-1]");
    TEST_ROUNDTRIP("[-123]");
    TEST_ROUNDTRIP("[-2147483648]");
}

TEST(Writer, UInt) {
    TEST_ROUNDTRIP("[0]");
    TEST_ROUNDTRIP("[1]");
    TEST_ROUNDTRIP("[123]");
    TEST_ROUNDTRIP("[2147483647]");
    TEST_ROUNDTRIP("[4294967295]");
}

TEST(Writer, Int64) {
    TEST_ROUNDTRIP("[-1234567890123456789]");
    TEST_ROUNDTRIP("[-9223372036854775808]");
}

TEST(Writer, Uint64) {
    TEST_ROUNDTRIP("[1234567890123456789]");
    TEST_ROUNDTRIP("[9223372036854775807]");
}

TEST(Writer, String) {
    TEST_ROUNDTRIP("[\"Hello\"]");
    TEST_ROUNDTRIP("[\"Hello\\u0000World\"]");
    TEST_ROUNDTRIP("[\"\\\"\\\\/\\b\\f\\n\\r\\t\"]");


    {
        StringBuffer buffer;
        Writer<StringBuffer> writer(buffer);
        writer.emplace_string(std::string("Hello\n"));
        EXPECT_STREQ("\"Hello\\n\"", buffer.get_string());
    }

}

TEST(Writer, Issue_889) {
    char buf[100] = "Hello";
    
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    writer.start_array();
    writer.emplace_string(buf);
    writer.end_array();
    
    EXPECT_STREQ("[\"Hello\"]", buffer.get_string());
    EXPECT_TRUE(writer.is_complete()); \
}

TEST(Writer, ScanWriteUnescapedString) {
    const char json[] = "[\" \\\"0123456789ABCDEF\"]";
    //                       ^ scanning stops here.
    char buffer2[sizeof(json) + 32];

    // Use different offset to test different alignments
    for (int i = 0; i < 32; i++) {
        char* p = buffer2 + i;
        memcpy(p, json, sizeof(json));
        TEST_ROUNDTRIP(p);
    }
}

TEST(Writer, Double) {
    TEST_ROUNDTRIP("[1.2345,1.2345678,0.123456789012,1234567.8]");
    TEST_ROUNDTRIP("0.0");
    TEST_ROUNDTRIP("-0.0"); // Issue #289
    TEST_ROUNDTRIP("1e30");
    TEST_ROUNDTRIP("1.0");
    TEST_ROUNDTRIP("5e-324"); // Min subnormal positive double
    TEST_ROUNDTRIP("2.225073858507201e-308"); // Max subnormal positive double
    TEST_ROUNDTRIP("2.2250738585072014e-308"); // Min normal positive double
    TEST_ROUNDTRIP("1.7976931348623157e308"); // Max double

}

// UTF8 -> TargetEncoding -> UTF8
template <typename TargetEncoding>
void TestTranscode(const char* json) {
    StringStream s(json);
    GenericStringBuffer<TargetEncoding> buffer;
    Writer<GenericStringBuffer<TargetEncoding>, UTF8<>, TargetEncoding> writer(buffer);
    Reader reader;
    reader.parse(s, writer);

    StringBuffer buffer2;
    Writer<StringBuffer> writer2(buffer2);
    GenericReader<TargetEncoding, UTF8<> > reader2;
    GenericStringStream<TargetEncoding> s2(buffer.get_string());
    reader2.parse(s2, writer2);

    EXPECT_STREQ(json, buffer2.get_string());
}

TEST(Writer, Transcode) {
    const char json[] = "{\"hello\":\"world\",\"t\":true,\"f\":false,\"n\":null,\"i\":123,\"pi\":3.1416,\"a\":[1,2,3],\"dollar\":\"\x24\",\"cents\":\"\xC2\xA2\",\"euro\":\"\xE2\x82\xAC\",\"gclef\":\"\xF0\x9D\x84\x9E\"}";

    // UTF8 -> UTF16 -> UTF8
    TestTranscode<UTF8<> >(json);

    // UTF8 -> ASCII -> UTF8
    TestTranscode<ASCII<> >(json);

    // UTF8 -> UTF16 -> UTF8
    TestTranscode<UTF16<> >(json);

    // UTF8 -> UTF32 -> UTF8
    TestTranscode<UTF32<> >(json);
}


TEST(Writer, StdOutputStream) {
    StringStream s("{ \"hello\" : \"world\", \"t\" : true , \"f\" : false, \"n\": null, \"i\":123, \"pi\": 3.1416, \"a\":[1, 2, 3], \"u64\": 1234567890123456789, \"i64\":-1234567890123456789 } ");
    
    std::stringstream ss;
    StdOutputStream os(ss);
    
    Writer<StdOutputStream> writer(os);

    Reader reader;
    reader.parse<0>(s, writer);
    
    std::string actual = ss.str();
    EXPECT_STREQ("{\"hello\":\"world\",\"t\":true,\"f\":false,\"n\":null,\"i\":123,\"pi\":3.1416,\"a\":[1,2,3],\"u64\":1234567890123456789,\"i64\":-1234567890123456789}", actual.c_str());
}

TEST(Writer, AssertRootMayBeAnyValue) {
#define T(x)\
    {\
        StringBuffer buffer;\
        Writer<StringBuffer> writer(buffer);\
        EXPECT_TRUE(x);\
    }
    T(writer.emplace_bool(false));
    T(writer.emplace_bool(true));
    T(writer.emplace_null());
    T(writer.emplace_int32(0));
    T(writer.emplace_uint32(0));
    T(writer.emplace_int64(0));
    T(writer.emplace_uint64(0));
    T(writer.emplace_double(0));
    T(writer.emplace_string("foo"));
#undef T
}

TEST(Writer, AssertIncorrectObjectLevel) {
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    writer.start_object();
    writer.end_object();
    ASSERT_THROW(writer.end_object(), AssertException);
}

TEST(Writer, AssertIncorrectArrayLevel) {
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    writer.start_array();
    writer.end_array();
    ASSERT_THROW(writer.end_array(), AssertException);
}

TEST(Writer, AssertIncorrectEndObject) {
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    writer.start_object();
    ASSERT_THROW(writer.end_array(), AssertException);
}

TEST(Writer, AssertIncorrectEndArray) {
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    writer.start_object();
    ASSERT_THROW(writer.end_array(), AssertException);
}

TEST(Writer, AssertObjectKeyNotString) {
#define T(x)\
    {\
        StringBuffer buffer;\
        Writer<StringBuffer> writer(buffer);\
        writer.start_object();\
        ASSERT_THROW(x, AssertException); \
    }
    T(writer.emplace_bool(false));
    T(writer.emplace_bool(true));
    T(writer.emplace_null());
    T(writer.emplace_int32(0));
    T(writer.emplace_uint32(0));
    T(writer.emplace_int64(0));
    T(writer.emplace_uint64(0));
    T(writer.emplace_double(0));
    T(writer.start_object());
    T(writer.start_array());
#undef T
}

TEST(Writer, AssertMultipleRoot) {
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);

    writer.start_object();
    writer.end_object();
    ASSERT_THROW(writer.start_object(), AssertException);

    writer.Reset(buffer);
    writer.emplace_null();
    ASSERT_THROW(writer.emplace_int32(0), AssertException);

    writer.Reset(buffer);
    writer.emplace_string("foo");
    ASSERT_THROW(writer.start_array(), AssertException);

    writer.Reset(buffer);
    writer.start_array();
    writer.end_array();
    //ASSERT_THROW(writer.emplace_double(3.14), AssertException);
}

TEST(Writer, RootObjectIsComplete) {
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    EXPECT_FALSE(writer.is_complete());
    writer.start_object();
    EXPECT_FALSE(writer.is_complete());
    writer.emplace_string("foo");
    EXPECT_FALSE(writer.is_complete());
    writer.emplace_int32(1);
    EXPECT_FALSE(writer.is_complete());
    writer.end_object();
    EXPECT_TRUE(writer.is_complete());
}

TEST(Writer, RootArrayIsComplete) {
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    EXPECT_FALSE(writer.is_complete());
    writer.start_array();
    EXPECT_FALSE(writer.is_complete());
    writer.emplace_string("foo");
    EXPECT_FALSE(writer.is_complete());
    writer.emplace_int32(1);
    EXPECT_FALSE(writer.is_complete());
    writer.end_array();
    EXPECT_TRUE(writer.is_complete());
}

TEST(Writer, RootValueIsComplete) {
#define T(x)\
    {\
        StringBuffer buffer;\
        Writer<StringBuffer> writer(buffer);\
        EXPECT_FALSE(writer.is_complete()); \
        x; \
        EXPECT_TRUE(writer.is_complete()); \
    }
    T(writer.emplace_null());
    T(writer.emplace_bool(true));
    T(writer.emplace_bool(false));
    T(writer.emplace_int32(0));
    T(writer.emplace_uint32(0));
    T(writer.emplace_int64(0));
    T(writer.emplace_uint64(0));
    T(writer.emplace_double(0));
    T(writer.emplace_string(""));
#undef T
}

TEST(Writer, InvalidEncoding) {
    // Fail in decoding invalid UTF-8 sequence http://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt
    {
        GenericStringBuffer<UTF16<> > buffer;
        Writer<GenericStringBuffer<UTF16<> >, UTF8<>, UTF16<> > writer(buffer);
        writer.start_array();
        EXPECT_FALSE(writer.emplace_string("\xfe"));
        EXPECT_FALSE(writer.emplace_string("\xff"));
        EXPECT_FALSE(writer.emplace_string("\xfe\xfe\xff\xff"));
        writer.end_array();
    }

    // Fail in encoding
    {
        StringBuffer buffer;
        Writer<StringBuffer, UTF32<> > writer(buffer);
        static const UTF32<>::char_type s[] = { 0x110000, 0 }; // Out of U+0000 to U+10FFFF
        EXPECT_FALSE(writer.emplace_string(s));
    }

    // Fail in unicode escaping in ASCII output
    {
        StringBuffer buffer;
        Writer<StringBuffer, UTF32<>, ASCII<> > writer(buffer);
        static const UTF32<>::char_type s[] = { 0x110000, 0 }; // Out of U+0000 to U+10FFFF
        EXPECT_FALSE(writer.emplace_string(s));
    }
}

TEST(Writer, ValidateEncoding) {
    {
        StringBuffer buffer;
        Writer<StringBuffer, UTF8<>, UTF8<>, CrtAllocator, kWriteValidateEncodingFlag> writer(buffer);
        writer.start_array();
        EXPECT_TRUE(writer.emplace_string("\x24"));             // Dollar sign U+0024
        EXPECT_TRUE(writer.emplace_string("\xC2\xA2"));         // Cents sign U+00A2
        EXPECT_TRUE(writer.emplace_string("\xE2\x82\xAC"));     // Euro sign U+20AC
        EXPECT_TRUE(writer.emplace_string("\xF0\x9D\x84\x9E")); // G clef sign U+1D11E
        EXPECT_TRUE(writer.emplace_string("\x01"));             // SOH control U+0001
        EXPECT_TRUE(writer.emplace_string("\x1B"));             // Escape control U+001B
        writer.end_array();
        EXPECT_STREQ("[\"\x24\",\"\xC2\xA2\",\"\xE2\x82\xAC\",\"\xF0\x9D\x84\x9E\",\"\\u0001\",\"\\u001B\"]", buffer.get_string());
    }

    // Fail in decoding invalid UTF-8 sequence http://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt
    {
        StringBuffer buffer;
        Writer<StringBuffer, UTF8<>, UTF8<>, CrtAllocator, kWriteValidateEncodingFlag> writer(buffer);
        writer.start_array();
        EXPECT_FALSE(writer.emplace_string("\xfe"));
        EXPECT_FALSE(writer.emplace_string("\xff"));
        EXPECT_FALSE(writer.emplace_string("\xfe\xfe\xff\xff"));
        writer.end_array();
    }
}

TEST(Writer, InvalidEventSequence) {
    // {]
    {
        StringBuffer buffer;
        Writer<StringBuffer> writer(buffer);
        writer.start_object();
        EXPECT_THROW(writer.end_array(), AssertException);
        EXPECT_FALSE(writer.is_complete());
    }

    // [}
    {
        StringBuffer buffer;
        Writer<StringBuffer> writer(buffer);
        writer.start_array();
        EXPECT_THROW(writer.end_object(), AssertException);
        EXPECT_FALSE(writer.is_complete());
    }

    // { 1: 
    {
        StringBuffer buffer;
        Writer<StringBuffer> writer(buffer);
        writer.start_object();
        EXPECT_THROW(writer.emplace_int32(1), AssertException);
        EXPECT_FALSE(writer.is_complete());
    }

    // { 'a' }
    {
        StringBuffer buffer;
        Writer<StringBuffer> writer(buffer);
        writer.start_object();
        writer.emplace_key("a");
        EXPECT_THROW(writer.end_object(), AssertException);
        EXPECT_FALSE(writer.is_complete());
    }

    // { 'a':'b','c' }
    {
        StringBuffer buffer;
        Writer<StringBuffer> writer(buffer);
        writer.start_object();
        writer.emplace_key("a");
        writer.emplace_string("b");
        writer.emplace_key("c");
        EXPECT_THROW(writer.end_object(), AssertException);
        EXPECT_FALSE(writer.is_complete());
    }
}

TEST(Writer, NaN) {
    double nan = std::numeric_limits<double>::quiet_NaN();

    EXPECT_TRUE(internal::Double(nan).IsNan());
    StringBuffer buffer;
    {
        Writer<StringBuffer> writer(buffer);
        EXPECT_FALSE(writer.emplace_double(nan));
    }
    {
        Writer<StringBuffer, UTF8<>, UTF8<>, CrtAllocator, kWriteNanAndInfFlag> writer(buffer);
        EXPECT_TRUE(writer.emplace_double(nan));
        EXPECT_STREQ("NaN", buffer.get_string());
    }
    GenericStringBuffer<UTF16<> > buffer2;
    Writer<GenericStringBuffer<UTF16<> > > writer2(buffer2);
    EXPECT_FALSE(writer2.emplace_double(nan));
}

TEST(Writer, NaNToNull) {
    double nan = std::numeric_limits<double>::quiet_NaN();

    EXPECT_TRUE(internal::Double(nan).IsNan());
    {
        StringBuffer buffer;
        Writer<StringBuffer, UTF8<>, UTF8<>, CrtAllocator, kWriteNanAndInfNullFlag> writer(buffer);
        EXPECT_TRUE(writer.emplace_double(nan));
        EXPECT_STREQ("null", buffer.get_string());
    }
}

TEST(Writer, Inf) {
    double inf = std::numeric_limits<double>::infinity();

    EXPECT_TRUE(internal::Double(inf).IsInf());
    StringBuffer buffer;
    {
        Writer<StringBuffer> writer(buffer);
        EXPECT_FALSE(writer.emplace_double(inf));
    }
    {
        Writer<StringBuffer> writer(buffer);
        EXPECT_FALSE(writer.emplace_double(-inf));
    }
    {
        Writer<StringBuffer, UTF8<>, UTF8<>, CrtAllocator, kWriteNanAndInfFlag> writer(buffer);
        EXPECT_TRUE(writer.emplace_double(inf));
    }
    {
        Writer<StringBuffer, UTF8<>, UTF8<>, CrtAllocator, kWriteNanAndInfFlag> writer(buffer);
        EXPECT_TRUE(writer.emplace_double(-inf));
    }
    EXPECT_STREQ("Infinity-Infinity", buffer.get_string());
}

TEST(Writer, InfToNull) {
    double inf = std::numeric_limits<double>::infinity();

    EXPECT_TRUE(internal::Double(inf).IsInf());
    {
        StringBuffer buffer;
        Writer<StringBuffer, UTF8<>, UTF8<>, CrtAllocator, kWriteNanAndInfNullFlag> writer(buffer);
        EXPECT_TRUE(writer.emplace_double(inf));
        EXPECT_STREQ("null", buffer.get_string());
    }
    {
        StringBuffer buffer;
        Writer<StringBuffer, UTF8<>, UTF8<>, CrtAllocator, kWriteNanAndInfNullFlag> writer(buffer);
        EXPECT_TRUE(writer.emplace_double(-inf));
        EXPECT_STREQ("null", buffer.get_string());
    }
}

TEST(Writer, raw_value) {
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    writer.start_object();
    writer.emplace_key("a");
    writer.emplace_int32(1);
    writer.emplace_key("raw");
    const char json[] = "[\"Hello\\nWorld\", 123.456]";
    writer.raw_value(json, strlen(json), kArrayType);
    writer.end_object();
    EXPECT_TRUE(writer.is_complete());
    EXPECT_STREQ("{\"a\":1,\"raw\":[\"Hello\\nWorld\", 123.456]}", buffer.get_string());
}

TEST(Write, RawValue_Issue1152) {
    {
        GenericStringBuffer<UTF32<> > sb;
        Writer<GenericStringBuffer<UTF32<> >, UTF8<>, UTF32<> > writer(sb);
        writer.raw_value("null", 4, kNullType);
        EXPECT_TRUE(writer.is_complete());
        const unsigned *out = sb.get_string();
        EXPECT_EQ(static_cast<unsigned>('n'), out[0]);
        EXPECT_EQ(static_cast<unsigned>('u'), out[1]);
        EXPECT_EQ(static_cast<unsigned>('l'), out[2]);
        EXPECT_EQ(static_cast<unsigned>('l'), out[3]);
        EXPECT_EQ(static_cast<unsigned>(0  ), out[4]);
    }

    {
        GenericStringBuffer<UTF8<> > sb;
        Writer<GenericStringBuffer<UTF8<> >, UTF16<>, UTF8<> > writer(sb);
        writer.raw_value(L"null", 4, kNullType);
        EXPECT_TRUE(writer.is_complete());
        EXPECT_STREQ("null", sb.get_string());
    }

    {
        // Fail in transcoding
        GenericStringBuffer<UTF16<> > buffer;
        Writer<GenericStringBuffer<UTF16<> >, UTF8<>, UTF16<> > writer(buffer);
        EXPECT_FALSE(writer.raw_value("\"\xfe\"", 3, kStringType));
    }

    {
        // Fail in encoding validation
        StringBuffer buffer;
        Writer<StringBuffer, UTF8<>, UTF8<>, CrtAllocator, kWriteValidateEncodingFlag> writer(buffer);
        EXPECT_FALSE(writer.raw_value("\"\xfe\"", 3, kStringType));
    }
}

static Writer<StringBuffer> WriterGen(StringBuffer &target) {
    Writer<StringBuffer> writer(target);
    writer.start_object();
    writer.emplace_key("a");
    writer.emplace_int32(1);
    return writer;
}

TEST(Writer, MoveCtor) {
    StringBuffer buffer;
    Writer<StringBuffer> writer(WriterGen(buffer));
    writer.end_object();
    EXPECT_TRUE(writer.is_complete());
    EXPECT_STREQ("{\"a\":1}", buffer.get_string());
}

#ifdef __clang__
MERAK_JSON_DIAG_POP
#endif
