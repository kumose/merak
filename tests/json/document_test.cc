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
#include <merak/json/document.h>
#include <merak/json/writer.h>
#include <merak/json/filereadstream.h>
#include <merak/json/encodedstream.h>
#include <merak/json/stringbuffer.h>
#include <sstream>
#include <algorithm>

#ifdef __clang__
MERAK_JSON_DIAG_PUSH
MERAK_JSON_DIAG_OFF(c++98-compat)
MERAK_JSON_DIAG_OFF(missing-variable-declarations)
#endif

using namespace merak::json;

template <typename DocumentType>
void ParseCheck(DocumentType& doc) {
    typedef typename DocumentType::ValueType ValueType;

    EXPECT_FALSE(doc.has_parse_error());
    if (doc.has_parse_error())
        printf("Error: %d at %zu\n", static_cast<int>(doc.get_parse_error()), doc.get_error_offset());
    EXPECT_TRUE(static_cast<ParseResult>(doc));

    EXPECT_TRUE(doc.is_object());

    EXPECT_TRUE(doc.has_member("hello"));
    const ValueType& hello = doc["hello"];
    EXPECT_TRUE(hello.is_string());
    EXPECT_STREQ("world", hello.get_string());

    EXPECT_TRUE(doc.has_member("t"));
    const ValueType& t = doc["t"];
    EXPECT_TRUE(t.is_true());

    EXPECT_TRUE(doc.has_member("f"));
    const ValueType& f = doc["f"];
    EXPECT_TRUE(f.is_false());

    EXPECT_TRUE(doc.has_member("n"));
    const ValueType& n = doc["n"];
    EXPECT_TRUE(n.is_null());

    EXPECT_TRUE(doc.has_member("i"));
    const ValueType& i = doc["i"];
    EXPECT_TRUE(i.is_number());
    EXPECT_EQ(123, i.get_int32());

    EXPECT_TRUE(doc.has_member("pi"));
    const ValueType& pi = doc["pi"];
    EXPECT_TRUE(pi.is_number());
    EXPECT_DOUBLE_EQ(3.1416, pi.get_double());

    EXPECT_TRUE(doc.has_member("a"));
    const ValueType& a = doc["a"];
    EXPECT_TRUE(a.is_array());
    EXPECT_EQ(4u, a.size());
    for (SizeType j = 0; j < 4; j++)
        EXPECT_EQ(j + 1, a[j].get_uint32());
}

template <typename Allocator, typename StackAllocator>
void ParseTest() {
    typedef GenericDocument<UTF8<>, Allocator, StackAllocator> DocumentType;
    DocumentType doc;

    const char* json = " { \"hello\" : \"world\", \"t\" : true , \"f\" : false, \"n\": null, \"i\":123, \"pi\": 3.1416, \"a\":[1, 2, 3, 4] } ";

    doc.parse(json);
    ParseCheck(doc);

    doc.SetNull();
    StringStream s(json);
    doc.template parse_stream<0>(s);
    ParseCheck(doc);

    doc.SetNull();

    doc.parse(json);
    ParseCheck(doc);

    // parse(const char_type*, size_t)
    size_t length = strlen(json);
    auto buffer = reinterpret_cast<char*>(malloc(length * 2));
    memcpy(buffer, json, length);
    memset(buffer + length, 'X', length);

    std::string s2(buffer, length); // backup buffer

    doc.SetNull();
    doc.parse(buffer, length);
    free(buffer);
    ParseCheck(doc);


    // parse(std::string)
    doc.SetNull();
    doc.parse(s2);
    ParseCheck(doc);

}

TEST(Document, parse) {
    ParseTest<MemoryPoolAllocator<>, CrtAllocator>();
    ParseTest<MemoryPoolAllocator<>, MemoryPoolAllocator<> >();
    ParseTest<CrtAllocator, MemoryPoolAllocator<> >();
    ParseTest<CrtAllocator, CrtAllocator>();
}

TEST(Document, UnchangedOnParseError) {
    Document doc;
    doc.set_array().push_back(0, doc.get_allocator());

    ParseResult noError;
    EXPECT_TRUE(noError);

    ParseResult err = doc.parse("{]");
    EXPECT_TRUE(doc.has_parse_error());
    EXPECT_NE(err, noError);
    EXPECT_NE(err.Code(), noError);
    EXPECT_NE(noError, doc.get_parse_error());
    EXPECT_EQ(err.Code(), doc.get_parse_error());
    EXPECT_EQ(err.Offset(), doc.get_error_offset());
    EXPECT_TRUE(doc.is_array());
    EXPECT_EQ(doc.size(), 1u);

    err = doc.parse("{}");
    EXPECT_FALSE(doc.has_parse_error());
    EXPECT_FALSE(err.IsError());
    EXPECT_TRUE(err);
    EXPECT_EQ(err, noError);
    EXPECT_EQ(err.Code(), noError);
    EXPECT_EQ(err.Code(), doc.get_parse_error());
    EXPECT_EQ(err.Offset(), doc.get_error_offset());
    EXPECT_TRUE(doc.is_object());
    EXPECT_EQ(doc.member_count(), 0u);
}

static FILE* OpenEncodedFile(const char* filename) {
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
        FILE *fp = fopen(buffer, "rb");
        if (fp)
            return fp;
    }
    return 0;
}

TEST(Document, Parse_Encoding) {
    const char* json = " { \"hello\" : \"world\", \"t\" : true , \"f\" : false, \"n\": null, \"i\":123, \"pi\": 3.1416, \"a\":[1, 2, 3, 4] } ";

    typedef GenericDocument<UTF16<> > DocumentType;
    DocumentType doc;
    
    // parse<unsigned, SourceEncoding>(const SourceEncoding::char_type*)
    // doc.parse<kParseDefaultFlags, UTF8<> >(json);
    // EXPECT_FALSE(doc.has_parse_error());
    // EXPECT_EQ(0, StrCmp(doc[L"hello"].get_string(), L"world"));

    // parse<unsigned, SourceEncoding>(const SourceEncoding::char_type*, size_t)
    size_t length = strlen(json);
    char* buffer = reinterpret_cast<char*>(malloc(length * 2));
    memcpy(buffer, json, length);
    memset(buffer + length, 'X', length);

    std::string s2(buffer, length); // backup buffer

    doc.SetNull();
    doc.parse<kParseDefaultFlags, UTF8<> >(buffer, length);
    free(buffer);
    EXPECT_FALSE(doc.has_parse_error());
    if (doc.has_parse_error())
        printf("Error: %d at %zu\n", static_cast<int>(doc.get_parse_error()), doc.get_error_offset());
    EXPECT_EQ(0, StrCmp(doc[L"hello"].get_string(), L"world"));


    // parse<unsigned, SourceEncoding>(std::string)
    doc.SetNull();

#if defined(_MSC_VER) && _MSC_VER < 1800
    doc.parse<kParseDefaultFlags, UTF8<> >(s2.c_str()); // VS2010 or below cannot handle templated function overloading. Use const char* instead.
#else
    doc.parse<kParseDefaultFlags, UTF8<> >(s2);
#endif
    EXPECT_FALSE(doc.has_parse_error());
    EXPECT_EQ(0, StrCmp(doc[L"hello"].get_string(), L"world"));

}

TEST(Document, ParseStream_EncodedInputStream) {
    // UTF8 -> UTF16
    FILE* fp = OpenEncodedFile("utf8.json");
    char buffer[256];
    FileReadStream bis(fp, buffer, sizeof(buffer));
    EncodedInputStream<UTF8<>, FileReadStream> eis(bis);

    GenericDocument<UTF16<> > d;
    d.parse_stream<0, UTF8<> >(eis);
    EXPECT_FALSE(d.has_parse_error());

    fclose(fp);

    wchar_t expected[] = L"I can eat glass and it doesn't hurt me.";
    GenericValue<UTF16<> >& v = d[L"en"];
    EXPECT_TRUE(v.is_string());
    EXPECT_EQ(sizeof(expected) / sizeof(wchar_t) - 1, v.get_string_length());
    EXPECT_EQ(0, StrCmp(expected, v.get_string()));

    // UTF16 -> UTF8 in memory
    StringBuffer bos;
    typedef EncodedOutputStream<UTF8<>, StringBuffer> OutputStream;
    OutputStream eos(bos, false);   // Not writing BOM
    {
        Writer<OutputStream, UTF16<>, UTF8<> > writer(eos);
        d.accept(writer);
    }

    // Condense the original file and compare.
    fp = OpenEncodedFile("utf8.json");
    FileReadStream is(fp, buffer, sizeof(buffer));
    Reader reader;
    StringBuffer bos2;
    Writer<StringBuffer> writer2(bos2);
    reader.parse(is, writer2);
    fclose(fp);

    EXPECT_EQ(bos.GetSize(), bos2.GetSize());
    EXPECT_EQ(0, memcmp(bos.get_string(), bos2.get_string(), bos2.GetSize()));
}

TEST(Document, swap) {
    Document d1;
    Document::AllocatorType& a = d1.get_allocator();

    d1.set_array().push_back(1, a).push_back(2, a);

    Value o;
    o.SetObject().add_member("a", 1, a);

    // swap between Document and Value
    d1.swap(o);
    EXPECT_TRUE(d1.is_object());
    EXPECT_TRUE(o.is_array());

    d1.swap(o);
    EXPECT_TRUE(d1.is_array());
    EXPECT_TRUE(o.is_object());

    o.swap(d1);
    EXPECT_TRUE(d1.is_object());
    EXPECT_TRUE(o.is_array());

    // swap between Document and Document
    Document d2;
    d2.set_array().push_back(3, a);
    d1.swap(d2);
    EXPECT_TRUE(d1.is_array());
    EXPECT_TRUE(d2.is_object());
    EXPECT_EQ(&d2.get_allocator(), &a);

    // reset value
    Value().swap(d1);
    EXPECT_TRUE(d1.is_null());

    // reset document, including allocator
    // so clear o before so that it doesnt contain dangling elements
    o.clear();
    Document().swap(d2);
    EXPECT_TRUE(d2.is_null());
    EXPECT_NE(&d2.get_allocator(), &a);

    // testing std::swap compatibility
    d1.SetBool(true);
    using std::swap;
    swap(d1, d2);
    EXPECT_TRUE(d1.is_null());
    EXPECT_TRUE(d2.is_true());

    swap(o, d2);
    EXPECT_TRUE(o.is_true());
    EXPECT_TRUE(d2.is_array());
}


// This should be slow due to assignment in inner-loop.
struct OutputStringStream : public std::ostringstream {
    typedef char char_type;

    virtual ~OutputStringStream();

    void put(char c) {
        std::ostringstream::put(c);
    }
    void write(const char_type* c, size_t length) {
        std::ostringstream::write(c, length);
    }
    OutputStringStream& flush() {
        return *this;
    }
};

OutputStringStream::~OutputStringStream() {}

TEST(Document, AcceptWriter) {
    Document doc;
    doc.parse(" { \"hello\" : \"world\", \"t\" : true , \"f\" : false, \"n\": null, \"i\":123, \"pi\": 3.1416, \"a\":[1, 2, 3, 4] } ");

    OutputStringStream os;
    Writer<OutputStringStream> writer(os);
    doc.accept(writer);

    EXPECT_EQ("{\"hello\":\"world\",\"t\":true,\"f\":false,\"n\":null,\"i\":123,\"pi\":3.1416,\"a\":[1,2,3,4]}", os.str());
}

TEST(Document, UserBuffer) {
    typedef GenericDocument<UTF8<>, MemoryPoolAllocator<>, MemoryPoolAllocator<> > DocumentType;
    char valueBuffer[4096];
    char parseBuffer[1024];
    MemoryPoolAllocator<> valueAllocator(valueBuffer, sizeof(valueBuffer));
    MemoryPoolAllocator<> parseAllocator(parseBuffer, sizeof(parseBuffer));
    DocumentType doc(&valueAllocator, sizeof(parseBuffer) / 2, &parseAllocator);
    doc.parse(" { \"hello\" : \"world\", \"t\" : true , \"f\" : false, \"n\": null, \"i\":123, \"pi\": 3.1416, \"a\":[1, 2, 3, 4] } ");
    EXPECT_FALSE(doc.has_parse_error());
    EXPECT_LE(valueAllocator.size(), sizeof(valueBuffer));
    EXPECT_LE(parseAllocator.size(), sizeof(parseBuffer));

    // Cover MemoryPoolAllocator::capacity()
    EXPECT_LE(valueAllocator.size(), valueAllocator.capacity());
    EXPECT_LE(parseAllocator.size(), parseAllocator.capacity());
}

// Issue 226: Value of string type should not point to NULL
TEST(Document, AssertAcceptInvalidNameType) {
    Document doc;
    doc.SetObject();
    doc.add_member("a", 0, doc.get_allocator());
    doc.find_member("a")->name.SetNull(); // Change name to non-string type.

    OutputStringStream os;
    Writer<OutputStringStream> writer(os);
    ASSERT_THROW(doc.accept(writer), AssertException);
}

// Issue 44:    set_string_raw doesn't work with wchar_t
TEST(Document, UTF16_Document) {
    GenericDocument< UTF16<> > json;
    json.parse<kParseValidateEncodingFlag>(L"[{\"created_at\":\"Wed Oct 30 17:13:20 +0000 2012\"}]");

    ASSERT_TRUE(json.is_array());
    GenericValue< UTF16<> >& v = json[0];
    ASSERT_TRUE(v.is_object());

    GenericValue< UTF16<> >& s = v[L"created_at"];
    ASSERT_TRUE(s.is_string());

    EXPECT_EQ(0, memcmp(L"Wed Oct 30 17:13:20 +0000 2012", s.get_string(), (s.get_string_length() + 1) * sizeof(wchar_t)));
}

#if 0 // Many old compiler does not support these. Turn it off temporaily.

#include <type_traits>

TEST(Document, Traits) {
    static_assert(std::is_constructible<Document>::value, "");
    static_assert(std::is_default_constructible<Document>::value, "");
#ifndef _MSC_VER
    static_assert(!std::is_copy_constructible<Document>::value, "");
#endif
    static_assert(std::is_move_constructible<Document>::value, "");

    static_assert(!std::is_nothrow_constructible<Document>::value, "");
    static_assert(!std::is_nothrow_default_constructible<Document>::value, "");
#ifndef _MSC_VER
    static_assert(!std::is_nothrow_copy_constructible<Document>::value, "");
    static_assert(std::is_nothrow_move_constructible<Document>::value, "");
#endif

    static_assert(std::is_assignable<Document,Document>::value, "");
#ifndef _MSC_VER
  static_assert(!std::is_copy_assignable<Document>::value, "");
#endif
    static_assert(std::is_move_assignable<Document>::value, "");

#ifndef _MSC_VER
    static_assert(std::is_nothrow_assignable<Document, Document>::value, "");
#endif
    static_assert(!std::is_nothrow_copy_assignable<Document>::value, "");
#ifndef _MSC_VER
    static_assert(std::is_nothrow_move_assignable<Document>::value, "");
#endif

    static_assert( std::is_destructible<Document>::value, "");
#ifndef _MSC_VER
    static_assert(std::is_nothrow_destructible<Document>::value, "");
#endif
}

#endif

template <typename Allocator>
struct DocumentMove: public ::testing::Test {
};

typedef ::testing::Types< CrtAllocator, MemoryPoolAllocator<> > MoveAllocatorTypes;
TYPED_TEST_CASE(DocumentMove, MoveAllocatorTypes);

TYPED_TEST(DocumentMove, MoveConstructor) {
    typedef TypeParam Allocator;
    typedef GenericDocument<UTF8<>, Allocator> D;
    Allocator allocator;

    D a(&allocator);
    a.parse("[\"one\", \"two\", \"three\"]");
    EXPECT_FALSE(a.has_parse_error());
    EXPECT_TRUE(a.is_array());
    EXPECT_EQ(3u, a.size());
    EXPECT_EQ(&a.get_allocator(), &allocator);

    // Document b(a); // does not compile (!is_copy_constructible)
    D b(std::move(a));
    EXPECT_TRUE(a.is_null());
    EXPECT_TRUE(b.is_array());
    EXPECT_EQ(3u, b.size());
    EXPECT_THROW(a.get_allocator(), AssertException);
    EXPECT_EQ(&b.get_allocator(), &allocator);

    b.parse("{\"Foo\": \"Bar\", \"Baz\": 42}");
    EXPECT_FALSE(b.has_parse_error());
    EXPECT_TRUE(b.is_object());
    EXPECT_EQ(2u, b.member_count());

    // Document c = a; // does not compile (!is_copy_constructible)
    D c = std::move(b);
    EXPECT_TRUE(b.is_null());
    EXPECT_TRUE(c.is_object());
    EXPECT_EQ(2u, c.member_count());
    EXPECT_THROW(b.get_allocator(), AssertException);
    EXPECT_EQ(&c.get_allocator(), &allocator);
}

TYPED_TEST(DocumentMove, MoveConstructorParseError) {
    typedef TypeParam Allocator;
    typedef GenericDocument<UTF8<>, Allocator> D;

    ParseResult noError;
    D a;
    a.parse("{ 4 = 4]");
    ParseResult error(a.get_parse_error(), a.get_error_offset());
    EXPECT_TRUE(a.has_parse_error());
    EXPECT_NE(error, noError);
    EXPECT_NE(error.Code(), noError);
    EXPECT_NE(error.Code(), noError.Code());
    EXPECT_NE(error.Offset(), noError.Offset());

    D b(std::move(a));
    EXPECT_FALSE(a.has_parse_error());
    EXPECT_TRUE(b.has_parse_error());
    EXPECT_EQ(a.get_parse_error(), noError);
    EXPECT_EQ(a.get_parse_error(), noError.Code());
    EXPECT_EQ(a.get_error_offset(), noError.Offset());
    EXPECT_EQ(b.get_parse_error(), error);
    EXPECT_EQ(b.get_parse_error(), error.Code());
    EXPECT_EQ(b.get_error_offset(), error.Offset());

    D c(std::move(b));
    EXPECT_FALSE(b.has_parse_error());
    EXPECT_TRUE(c.has_parse_error());
    EXPECT_EQ(b.get_parse_error(), noError.Code());
    EXPECT_EQ(c.get_parse_error(), error.Code());
    EXPECT_EQ(b.get_error_offset(), noError.Offset());
    EXPECT_EQ(c.get_error_offset(), error.Offset());
}

// This test does not properly use parsing, just for testing.
// It must call clear_stack() explicitly to prevent memory leak.
// But here we cannot as clear_stack() is private.
#if 0
TYPED_TEST(DocumentMove, MoveConstructorStack) {
    typedef TypeParam Allocator;
    typedef UTF8<> Encoding;
    typedef GenericDocument<Encoding, Allocator> Document;

    Document a;
    size_t defaultCapacity = a.get_stack_capacity();

    // Trick Document into getting get_stack_capacity() to return non-zero
    typedef GenericReader<Encoding, Encoding, Allocator> Reader;
    Reader reader(&a.get_allocator());
    GenericStringStream<Encoding> is("[\"one\", \"two\", \"three\"]");
    reader.template parse<kParseDefaultFlags>(is, a);
    size_t capacity = a.get_stack_capacity();
    EXPECT_GT(capacity, 0u);

    Document b(std::move(a));
    EXPECT_EQ(a.get_stack_capacity(), defaultCapacity);
    EXPECT_EQ(b.get_stack_capacity(), capacity);

    Document c = std::move(b);
    EXPECT_EQ(b.get_stack_capacity(), defaultCapacity);
    EXPECT_EQ(c.get_stack_capacity(), capacity);
}
#endif

TYPED_TEST(DocumentMove, MoveAssignment) {
    typedef TypeParam Allocator;
    typedef GenericDocument<UTF8<>, Allocator> D;
    Allocator allocator;

    D a(&allocator);
    a.parse("[\"one\", \"two\", \"three\"]");
    EXPECT_FALSE(a.has_parse_error());
    EXPECT_TRUE(a.is_array());
    EXPECT_EQ(3u, a.size());
    EXPECT_EQ(&a.get_allocator(), &allocator);

    // Document b; b = a; // does not compile (!is_copy_assignable)
    D b;
    b = std::move(a);
    EXPECT_TRUE(a.is_null());
    EXPECT_TRUE(b.is_array());
    EXPECT_EQ(3u, b.size());
    EXPECT_THROW(a.get_allocator(), AssertException);
    EXPECT_EQ(&b.get_allocator(), &allocator);

    b.parse("{\"Foo\": \"Bar\", \"Baz\": 42}");
    EXPECT_FALSE(b.has_parse_error());
    EXPECT_TRUE(b.is_object());
    EXPECT_EQ(2u, b.member_count());

    // Document c; c = a; // does not compile (see static_assert)
    D c;
    c = std::move(b);
    EXPECT_TRUE(b.is_null());
    EXPECT_TRUE(c.is_object());
    EXPECT_EQ(2u, c.member_count());
    EXPECT_THROW(b.get_allocator(), AssertException);
    EXPECT_EQ(&c.get_allocator(), &allocator);
}

TYPED_TEST(DocumentMove, MoveAssignmentParseError) {
    typedef TypeParam Allocator;
    typedef GenericDocument<UTF8<>, Allocator> D;

    ParseResult noError;
    D a;
    a.parse("{ 4 = 4]");
    ParseResult error(a.get_parse_error(), a.get_error_offset());
    EXPECT_TRUE(a.has_parse_error());
    EXPECT_NE(error.Code(), noError.Code());
    EXPECT_NE(error.Offset(), noError.Offset());

    D b;
    b = std::move(a);
    EXPECT_FALSE(a.has_parse_error());
    EXPECT_TRUE(b.has_parse_error());
    EXPECT_EQ(a.get_parse_error(), noError.Code());
    EXPECT_EQ(b.get_parse_error(), error.Code());
    EXPECT_EQ(a.get_error_offset(), noError.Offset());
    EXPECT_EQ(b.get_error_offset(), error.Offset());

    D c;
    c = std::move(b);
    EXPECT_FALSE(b.has_parse_error());
    EXPECT_TRUE(c.has_parse_error());
    EXPECT_EQ(b.get_parse_error(), noError.Code());
    EXPECT_EQ(c.get_parse_error(), error.Code());
    EXPECT_EQ(b.get_error_offset(), noError.Offset());
    EXPECT_EQ(c.get_error_offset(), error.Offset());
}

// This test does not properly use parsing, just for testing.
// It must call clear_stack() explicitly to prevent memory leak.
// But here we cannot as clear_stack() is private.
#if 0
TYPED_TEST(DocumentMove, MoveAssignmentStack) {
    typedef TypeParam Allocator;
    typedef UTF8<> Encoding;
    typedef GenericDocument<Encoding, Allocator> D;

    D a;
    size_t defaultCapacity = a.get_stack_capacity();

    // Trick Document into getting get_stack_capacity() to return non-zero
    typedef GenericReader<Encoding, Encoding, Allocator> Reader;
    Reader reader(&a.get_allocator());
    GenericStringStream<Encoding> is("[\"one\", \"two\", \"three\"]");
    reader.template parse<kParseDefaultFlags>(is, a);
    size_t capacity = a.get_stack_capacity();
    EXPECT_GT(capacity, 0u);

    D b;
    b = std::move(a);
    EXPECT_EQ(a.get_stack_capacity(), defaultCapacity);
    EXPECT_EQ(b.get_stack_capacity(), capacity);

    D c;
    c = std::move(b);
    EXPECT_EQ(b.get_stack_capacity(), defaultCapacity);
    EXPECT_EQ(c.get_stack_capacity(), capacity);
}
#endif


// Issue 22: Memory corruption via operator=
// Fixed by making unimplemented assignment operator private.
//TEST(Document, Assignment) {
//  Document d1;
//  Document d2;
//  d1 = d2;
//}

#ifdef __clang__
MERAK_JSON_DIAG_POP
#endif
