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

#include <tests/pertf/perftest.h>

#if TEST_RAPIDJSON

#include <merak/json/json_internal.h>
#include <merak/json/document.h>
#include <merak/json/prettywriter.h>
#include <merak/json/stringbuffer.h>
#include <merak/json/filereadstream.h>
#include <merak/json/std_input_stream.h>
#include <merak/json/encodedstream.h>
#include <merak/json/memorystream.h>

#include <fstream>
#include <vector>

#ifdef MERAK_JSON_SSE2
#define SIMD_SUFFIX(name) name##_SSE2
#elif defined(MERAK_JSON_SSE42)
#define SIMD_SUFFIX(name) name##_SSE42
#elif defined(MERAK_JSON_NEON)
#define SIMD_SUFFIX(name) name##_NEON
#else
#define SIMD_SUFFIX(name) name
#endif

using namespace merak::json;

class RapidJson : public PerfTest {
public:
    RapidJson() : temp_(), doc_() {}

    virtual void SetUp() {
        PerfTest::SetUp();

        // temp buffer for insitu parsing.
        temp_ = (char *)malloc(length_ + 1);

        // parse as a document
        EXPECT_FALSE(doc_.parse(json_).has_parse_error());

        for (size_t i = 0; i < 8; i++)
            EXPECT_FALSE(typesDoc_[i].parse(types_[i]).has_parse_error());
    }

    virtual void TearDown() {
        PerfTest::TearDown();
        free(temp_);
    }

private:
    RapidJson(const RapidJson&);
    RapidJson& operator=(const RapidJson&);

protected:
    char *temp_;
    Document doc_;
    Document typesDoc_[8];
};


TEST_F(RapidJson, SIMD_SUFFIX(ReaderParse_DummyHandler)) {
    for (size_t i = 0; i < kTrialCount; i++) {
        StringStream s(json_);
        BaseReaderHandler<> h;
        Reader reader;
        EXPECT_TRUE(reader.parse(s, h));
    }
}

#define TEST_TYPED(index, Name)\
TEST_F(RapidJson, SIMD_SUFFIX(ReaderParse_DummyHandler_##Name)) {\
    for (size_t i = 0; i < kTrialCount * 10; i++) {\
        StringStream s(types_[index]);\
        BaseReaderHandler<> h;\
        Reader reader;\
        EXPECT_TRUE(reader.parse(s, h));\
    }\
}

TEST_TYPED(0, Booleans)
TEST_TYPED(1, Floats)
TEST_TYPED(2, Guids)
TEST_TYPED(3, Integers)
TEST_TYPED(4, Mixed)
TEST_TYPED(5, Nulls)
TEST_TYPED(6, Paragraphs)

#undef TEST_TYPED

TEST_F(RapidJson, SIMD_SUFFIX(ReaderParse_DummyHandler_FullPrecision)) {
    for (size_t i = 0; i < kTrialCount; i++) {
        StringStream s(json_);
        BaseReaderHandler<> h;
        Reader reader;
        EXPECT_TRUE(reader.parse<kParseFullPrecisionFlag>(s, h));
    }
}

TEST_F(RapidJson, SIMD_SUFFIX(ReaderParseIterative_DummyHandler)) {
    for (size_t i = 0; i < kTrialCount; i++) {
        StringStream s(json_);
        BaseReaderHandler<> h;
        Reader reader;
        EXPECT_TRUE(reader.parse<kParseIterativeFlag>(s, h));
    }
}


TEST_F(RapidJson, SIMD_SUFFIX(ReaderParseIterativePull_DummyHandler)) {
    for (size_t i = 0; i < kTrialCount; i++) {
        StringStream s(json_);
        BaseReaderHandler<> h;
        Reader reader;
        reader.IterativeParseInit();
        while (!reader.IterativeParseComplete()) {
            if (!reader.IterativeParseNext<kParseDefaultFlags>(s, h))
                break;
        }
        EXPECT_FALSE(reader.has_parse_error());
    }
}

TEST_F(RapidJson, SIMD_SUFFIX(ReaderParse_DummyHandler_ValidateEncoding)) {
    for (size_t i = 0; i < kTrialCount; i++) {
        StringStream s(json_);
        BaseReaderHandler<> h;
        Reader reader;
        EXPECT_TRUE(reader.parse<kParseValidateEncodingFlag>(s, h));
    }
}


TEST_F(RapidJson, SIMD_SUFFIX(DocumentParse_MemoryPoolAllocator)) {
    for (size_t i = 0; i < kTrialCount; i++) {
        Document doc;
        doc.parse(json_);
        ASSERT_TRUE(doc.is_object());
    }
}

TEST_F(RapidJson, SIMD_SUFFIX(DocumentParseLength_MemoryPoolAllocator)) {
    for (size_t i = 0; i < kTrialCount; i++) {
        Document doc;
        doc.parse(json_, length_);
        ASSERT_TRUE(doc.is_object());
    }
}


TEST_F(RapidJson, SIMD_SUFFIX(DocumentParseStdString_MemoryPoolAllocator)) {
    const std::string s(json_, length_);
    for (size_t i = 0; i < kTrialCount; i++) {
        Document doc;
        doc.parse(s);
        ASSERT_TRUE(doc.is_object());
    }
}


TEST_F(RapidJson, SIMD_SUFFIX(DocumentParseIterative_MemoryPoolAllocator)) {
    for (size_t i = 0; i < kTrialCount; i++) {
        Document doc;
        doc.parse<kParseIterativeFlag>(json_);
        ASSERT_TRUE(doc.is_object());
    }
}

TEST_F(RapidJson, SIMD_SUFFIX(DocumentParse_CrtAllocator)) {
    for (size_t i = 0; i < kTrialCount; i++) {
        memcpy(temp_, json_, length_ + 1);
        GenericDocument<UTF8<>, CrtAllocator> doc;
        doc.parse(temp_);
        ASSERT_TRUE(doc.is_object());
    }
}

TEST_F(RapidJson, SIMD_SUFFIX(DocumentParseEncodedInputStream_MemoryStream)) {
    for (size_t i = 0; i < kTrialCount; i++) {
        MemoryStream ms(json_, length_);
        EncodedInputStream<UTF8<>, MemoryStream> is(ms);
        Document doc;
        doc.parse_stream<0, UTF8<> >(is);
        ASSERT_TRUE(doc.is_object());
    }
}


template<typename T>
size_t Traverse(const T& value) {
    size_t count = 1;
    switch(value.get_type()) {
        case kObjectType:
            for (typename T::ConstMemberIterator itr = value.member_begin(); itr != value.member_end(); ++itr) {
                count++;    // name
                count += Traverse(itr->value);
            }
            break;

        case kArrayType:
            for (typename T::ConstValueIterator itr = value.begin(); itr != value.end(); ++itr)
                count += Traverse(*itr);
            break;

        default:
            // Do nothing.
            break;
    }
    return count;
}

TEST_F(RapidJson, DocumentTraverse) {
    for (size_t i = 0; i < kTrialCount; i++) {
        size_t count = Traverse(doc_);
        EXPECT_EQ(4339u, count);
        //if (i == 0)
        //  std::cout << count << std::endl;
    }
}

#ifdef __GNUC__
MERAK_JSON_DIAG_PUSH
MERAK_JSON_DIAG_OFF(effc++)
#endif

struct ValueCounter : public BaseReaderHandler<> {
    ValueCounter() : count_(1) {}   // root

    bool end_object(SizeType memberCount) { count_ += memberCount * 2; return true; }
    bool end_array(SizeType elementCount) { count_ += elementCount; return true; }

    SizeType count_;
};

#ifdef __GNUC__
MERAK_JSON_DIAG_POP
#endif

TEST_F(RapidJson, DocumentAccept) {
    for (size_t i = 0; i < kTrialCount; i++) {
        ValueCounter counter;
        doc_.accept(counter);
        EXPECT_EQ(4339u, counter.count_);
    }
}

TEST_F(RapidJson, DocumentFind) {
    typedef Document::ValueType ValueType;
    typedef ValueType::ConstMemberIterator ConstMemberIterator;
    const Document &doc = typesDoc_[7]; // alotofkeys.json
    if (doc.is_object()) {
        std::vector<const ValueType*> keys;
        for (ConstMemberIterator it = doc.member_begin(); it != doc.member_end(); ++it) {
            keys.push_back(&it->name);
        }
        for (size_t i = 0; i < kTrialCount; i++) {
            for (size_t j = 0; j < keys.size(); j++) {
                EXPECT_TRUE(doc.find_member(*keys[j]) != doc.member_end());
            }
        }
    }
}

struct NullStream {
    typedef char char_type;

    NullStream() /*: length_(0)*/ {}
    void put(char_type) { /*++length_;*/ }
    void write(const char_type* c, size_t length) {
    }
    NullStream flush() {
        return *this;
    }
    //size_t length_;
};

TEST_F(RapidJson, Writer_NullStream) {
    for (size_t i = 0; i < kTrialCount; i++) {
        NullStream s;
        Writer<NullStream> writer(s);
        doc_.accept(writer);
        //if (i == 0)
        //  std::cout << s.length_ << std::endl;
    }
}

TEST_F(RapidJson, SIMD_SUFFIX(Writer_StringBuffer)) {
    for (size_t i = 0; i < kTrialCount; i++) {
        StringBuffer s(0, 1024 * 1024);
        Writer<StringBuffer> writer(s);
        doc_.accept(writer);
        const char* str = s.get_string();
        (void)str;
        //if (i == 0)
        //  std::cout << strlen(str) << std::endl;
    }
}

#define TEST_TYPED(index, Name)\
TEST_F(RapidJson, SIMD_SUFFIX(Writer_StringBuffer_##Name)) {\
    for (size_t i = 0; i < kTrialCount * 10; i++) {\
        StringBuffer s(0, 1024 * 1024);\
        Writer<StringBuffer> writer(s);\
        typesDoc_[index].accept(writer);\
        const char* str = s.get_string();\
        (void)str;\
    }\
}

TEST_TYPED(0, Booleans)
TEST_TYPED(1, Floats)
TEST_TYPED(2, Guids)
TEST_TYPED(3, Integers)
TEST_TYPED(4, Mixed)
TEST_TYPED(5, Nulls)
TEST_TYPED(6, Paragraphs)

#undef TEST_TYPED

TEST_F(RapidJson, SIMD_SUFFIX(PrettyWriter_StringBuffer)) {
    for (size_t i = 0; i < kTrialCount; i++) {
        StringBuffer s(0, 2048 * 1024);
        PrettyWriter<StringBuffer> writer(s);
        writer.SetIndent(' ', 1);
        doc_.accept(writer);
        const char* str = s.get_string();
        (void)str;
        //if (i == 0)
        //  std::cout << strlen(str) << std::endl;
    }
}

TEST_F(RapidJson, internal_Pow10) {
    double sum = 0;
    for (size_t i = 0; i < kTrialCount * kTrialCount; i++)
        sum += internal::Pow10(int(i & 255));
    EXPECT_GT(sum, 0.0);
}

TEST_F(RapidJson, SkipWhitespace_Basic) {
    for (size_t i = 0; i < kTrialCount; i++) {
        merak::json::StringStream s(whitespace_);
        while (s.peek() == ' ' || s.peek() == '\n' || s.peek() == '\r' || s.peek() == '\t')
            s.get();
        ASSERT_EQ('[', s.peek());
    }
}

TEST_F(RapidJson, SIMD_SUFFIX(SkipWhitespace)) {
    for (size_t i = 0; i < kTrialCount; i++) {
        merak::json::StringStream s(whitespace_);
        merak::json::SkipWhitespace(s);
        ASSERT_EQ('[', s.peek());
    }
}

TEST_F(RapidJson, SkipWhitespace_strspn) {
    for (size_t i = 0; i < kTrialCount; i++) {
        const char* s = whitespace_ + std::strspn(whitespace_, " \t\r\n");
        ASSERT_EQ('[', *s);
    }
}

TEST_F(RapidJson, UTF8_Validate) {
    NullStream os;

    for (size_t i = 0; i < kTrialCount; i++) {
        StringStream is(json_);
        bool result = true;
        while (is.peek() != '\0')
            result &= UTF8<>::Validate(is, os);
        EXPECT_TRUE(result);
    }
}

TEST_F(RapidJson, FileReadStream) {
    for (size_t i = 0; i < kTrialCount; i++) {
        FILE *fp = fopen(filename_, "rb");
        char buffer[65536];
        FileReadStream s(fp, buffer, sizeof(buffer));
        while (s.get() != '\0')
            ;
        fclose(fp);
    }
}

TEST_F(RapidJson, SIMD_SUFFIX(ReaderParse_DummyHandler_FileReadStream)) {
    for (size_t i = 0; i < kTrialCount; i++) {
        FILE *fp = fopen(filename_, "rb");
        char buffer[65536];
        FileReadStream s(fp, buffer, sizeof(buffer));
        BaseReaderHandler<> h;
        Reader reader;
        reader.parse(s, h);
        fclose(fp);
    }
}

TEST_F(RapidJson, FileInputStream) {
    for (size_t i = 0; i < kTrialCount; i++) {
        std::ifstream is(filename_, std::ios::in | std::ios::binary);
        FileInputStream isw(is, 65536);
        while (isw.get() != '\0')
            ;
        is.close();
    }
}

TEST_F(RapidJson, IStreamWrapper_Unbuffered) {
    for (size_t i = 0; i < kTrialCount; i++) {
        std::ifstream is(filename_, std::ios::in | std::ios::binary);
        FileInputStream isw(is);
        while (isw.get() != '\0')
            ;
        is.close();
    }
}

TEST_F(RapidJson, IStreamWrapper_Setbuffered) {
    for (size_t i = 0; i < kTrialCount; i++) {
        std::ifstream is;
        char buffer[65536];
        is.rdbuf()->pubsetbuf(buffer, sizeof(buffer));
        is.open(filename_, std::ios::in | std::ios::binary);
        FileInputStream isw(is);
        while (isw.get() != '\0')
            ;
        is.close();
    }
}

TEST_F(RapidJson, SIMD_SUFFIX(ReaderParse_DummyHandler_IStreamWrapper)) {
    for (size_t i = 0; i < kTrialCount; i++) {
        std::ifstream is(filename_, std::ios::in | std::ios::binary);
        FileInputStream isw(is, 65536);
        BaseReaderHandler<> h;
        Reader reader;
        reader.parse(isw, h);
        is.close();
    }
}

TEST_F(RapidJson, SIMD_SUFFIX(ReaderParse_DummyHandler_IStreamWrapper_Unbuffered)) {
    for (size_t i = 0; i < kTrialCount; i++) {
        std::ifstream is(filename_, std::ios::in | std::ios::binary);
        FileInputStream isw(is);
        BaseReaderHandler<> h;
        Reader reader;
        reader.parse(isw, h);
        is.close();
    }
}

TEST_F(RapidJson, SIMD_SUFFIX(ReaderParse_DummyHandler_IStreamWrapper_Setbuffered)) {
    for (size_t i = 0; i < kTrialCount; i++) {
        std::ifstream is;
        char buffer[65536];
        is.rdbuf()->pubsetbuf(buffer, sizeof(buffer));
        is.open(filename_, std::ios::in | std::ios::binary);
        FileInputStream isw(is);
        BaseReaderHandler<> h;
        Reader reader;
        reader.parse(isw, h);
        is.close();
    }
}

TEST_F(RapidJson, StringBuffer) {
    StringBuffer sb;
    for (int i = 0; i < 32 * 1024 * 1024; i++)
        sb.put(i & 0x7f);
}

#endif // TEST_RAPIDJSON
