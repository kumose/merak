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
#include <merak/json/filereadstream.h>
#include <merak/json/filewritestream.h>
#include <merak/json/encodedstream.h>
#include <merak/json/stringbuffer.h>
#include <merak/json/memorystream.h>
#include <merak/json/memorybuffer.h>

using namespace merak::json;

class EncodedStreamTest : public ::testing::Test {
public:
    EncodedStreamTest() : json_(), length_() {}
    virtual ~EncodedStreamTest();

    virtual void SetUp() {
        json_ = ReadFile("utf8.json", true, &length_);
    }

    virtual void TearDown() {
        free(json_);
        json_ = 0;
    }

private:
    EncodedStreamTest(const EncodedStreamTest&);
    EncodedStreamTest& operator=(const EncodedStreamTest&);
    
protected:
    static FILE* Open(const char* filename) {
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

    static char *ReadFile(const char* filename, bool appendPath, size_t* outLength) {
        FILE *fp = appendPath ? Open(filename) : fopen(filename, "rb");

        if (!fp) {
            *outLength = 0;
            return 0;
        }

        fseek(fp, 0, SEEK_END);
        *outLength = static_cast<size_t>(ftell(fp));
        fseek(fp, 0, SEEK_SET);
        char* buffer = static_cast<char*>(malloc(*outLength + 1));
        size_t readLength = fread(buffer, 1, *outLength, fp);
        buffer[readLength] = '\0';
        fclose(fp);
        return buffer;
    }

    template <typename FileEncoding, typename MemoryEncoding>
    void TestEncodedInputStream(const char* filename) {
        // Test FileReadStream
        {
            char buffer[16];
            FILE *fp = Open(filename);
            ASSERT_TRUE(fp != 0);
            FileReadStream fs(fp, buffer, sizeof(buffer));
            EncodedInputStream<FileEncoding, FileReadStream> eis(fs);
            StringStream s(json_);

            while (eis.peek() != '\0') {
                unsigned expected, actual;
                EXPECT_TRUE(UTF8<>::Decode(s, &expected));
                EXPECT_TRUE(MemoryEncoding::Decode(eis, &actual));
                EXPECT_EQ(expected, actual);
            }
            EXPECT_EQ('\0', s.peek());
            fclose(fp);
        }

        // Test MemoryStream
        {
            size_t size;
            char* data = ReadFile(filename, true, &size);
            MemoryStream ms(data, size);
            EncodedInputStream<FileEncoding, MemoryStream> eis(ms);
            StringStream s(json_);

            while (eis.peek() != '\0') {
                unsigned expected, actual;
                EXPECT_TRUE(UTF8<>::Decode(s, &expected));
                EXPECT_TRUE(MemoryEncoding::Decode(eis, &actual));
                EXPECT_EQ(expected, actual);
            }
            EXPECT_EQ('\0', s.peek());
            EXPECT_EQ(size, eis.tellg());
            free(data);
        }
    }

    template <typename FileEncoding, typename MemoryEncoding>
    void TestEncodedOutputStream(const char* expectedFilename, bool putBOM) {
        // Test FileWriteStream
        {
            char filename[L_tmpnam];
            FILE* fp = TempFile(filename);
            char buffer[16];
            FileWriteStream os(fp, buffer, sizeof(buffer));
            EncodedOutputStream<FileEncoding, FileWriteStream> eos(os, putBOM);
            StringStream s(json_);
            while (s.peek() != '\0') {
                bool success = Transcoder<UTF8<>, MemoryEncoding>::Transcode(s, eos);
                EXPECT_TRUE(success);
            }
            eos.flush();
            fclose(fp);
            EXPECT_TRUE(CompareFile(filename, expectedFilename));
            remove(filename);
        }

        // Test MemoryBuffer
        {
            MemoryBuffer mb;
            EncodedOutputStream<FileEncoding, MemoryBuffer> eos(mb, putBOM);
            StringStream s(json_);
            while (s.peek() != '\0') {
                bool success = Transcoder<UTF8<>, MemoryEncoding>::Transcode(s, eos);
                EXPECT_TRUE(success);
            }
            eos.flush();
            EXPECT_TRUE(CompareBufferFile(mb.GetBuffer(), mb.GetSize(), expectedFilename));
        }
    }


    bool CompareFile(const char* filename, const char* expectedFilename) {
        size_t actualLength, expectedLength;
        char* actualBuffer = ReadFile(filename, false, &actualLength);
        char* expectedBuffer = ReadFile(expectedFilename, true, &expectedLength);
        bool ret = (expectedLength == actualLength) && memcmp(expectedBuffer, actualBuffer, actualLength) == 0;
        free(actualBuffer);
        free(expectedBuffer);
        return ret;
    }

    bool CompareBufferFile(const char* actualBuffer, size_t actualLength, const char* expectedFilename) {
        size_t expectedLength;
        char* expectedBuffer = ReadFile(expectedFilename, true, &expectedLength);
        bool ret = (expectedLength == actualLength) && memcmp(expectedBuffer, actualBuffer, actualLength) == 0;
        free(expectedBuffer);
        return ret;
    }

    char *json_;
    size_t length_;
};

EncodedStreamTest::~EncodedStreamTest() {}

TEST_F(EncodedStreamTest, EncodedInputStream) {
    TestEncodedInputStream<UTF8<>,    UTF8<>  >("utf8.json");
    TestEncodedInputStream<UTF8<>,    UTF8<>  >("utf8bom.json");
    TestEncodedInputStream<UTF16LE<>, UTF16<> >("utf16le.json");
    TestEncodedInputStream<UTF16LE<>, UTF16<> >("utf16lebom.json");
    TestEncodedInputStream<UTF16BE<>, UTF16<> >("utf16be.json");
    TestEncodedInputStream<UTF16BE<>, UTF16<> >("utf16bebom.json");
    TestEncodedInputStream<UTF32LE<>, UTF32<> >("utf32le.json");
    TestEncodedInputStream<UTF32LE<>, UTF32<> >("utf32lebom.json");
    TestEncodedInputStream<UTF32BE<>, UTF32<> >("utf32be.json");
    TestEncodedInputStream<UTF32BE<>, UTF32<> >("utf32bebom.json");
}


TEST_F(EncodedStreamTest, EncodedOutputStream) {
    TestEncodedOutputStream<UTF8<>,     UTF8<>  >("utf8.json",      false);
    TestEncodedOutputStream<UTF8<>,     UTF8<>  >("utf8bom.json",   true);
    TestEncodedOutputStream<UTF16LE<>,  UTF16<> >("utf16le.json",   false);
    TestEncodedOutputStream<UTF16LE<>,  UTF16<> >("utf16lebom.json",true);
    TestEncodedOutputStream<UTF16BE<>,  UTF16<> >("utf16be.json",   false);
    TestEncodedOutputStream<UTF16BE<>,  UTF16<> >("utf16bebom.json",true);
    TestEncodedOutputStream<UTF32LE<>,  UTF32<> >("utf32le.json",   false);
    TestEncodedOutputStream<UTF32LE<>,  UTF32<> >("utf32lebom.json",true);
    TestEncodedOutputStream<UTF32BE<>,  UTF32<> >("utf32be.json",   false);
    TestEncodedOutputStream<UTF32BE<>,  UTF32<> >("utf32bebom.json",true);
}

