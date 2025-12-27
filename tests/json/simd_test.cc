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

// Since Travis CI installs old Valgrind 3.7.0, which fails with some SSE4.2
// The unit tests prefix with SIMD should be skipped by Valgrind test

// __SSE2__ and __SSE4_2__ are recognized by gcc, clang, and the Intel compiler.
// We use -march=native with gmake to enable -msse2 and -msse4.2, if supported.
#if defined(__SSE4_2__)
#  define MERAK_JSON_SSE42
#elif defined(__SSE2__)
#  define MERAK_JSON_SSE2
#elif defined(__ARM_NEON)
#  define MERAK_JSON_NEON
#endif

#include <tests/json/unittest.h>

#include <merak/json/reader.h>
#include <merak/json/writer.h>

#ifdef __GNUC__
MERAK_JSON_DIAG_PUSH
MERAK_JSON_DIAG_OFF(effc++)
#endif

using namespace merak::json;

#ifdef MERAK_JSON_SSE2
#define SIMD_SUFFIX(name) name##_SSE2
#elif defined(MERAK_JSON_SSE42)
#define SIMD_SUFFIX(name) name##_SSE42
#elif defined(MERAK_JSON_NEON)
#define SIMD_SUFFIX(name) name##_NEON
#else
#define SIMD_SUFFIX(name) name
#endif

#define SIMD_SIZE_ALIGN(n) ((size_t(n) + 15) & ~size_t(15))

template <typename StreamType>
void TestSkipWhitespace() {
    for (size_t step = 1; step < 32; step++) {
        char buffer[SIMD_SIZE_ALIGN(1025)];
        for (size_t i = 0; i < 1024; i++)
            buffer[i] = " \t\r\n"[i % 4];
        for (size_t i = 0; i < 1024; i += step)
            buffer[i] = 'X';
        buffer[1024] = '\0';

        StreamType s(buffer);
        size_t i = 0;
        for (;;) {
            SkipWhitespace(s);
            if (s.peek() == '\0')
                break;
            EXPECT_EQ(i, s.tellg());
            EXPECT_EQ('X', s.get());
            i += step;
        }
    }
}

TEST(SIMD, SIMD_SUFFIX(SkipWhitespace)) {
    TestSkipWhitespace<StringStream>();
    TestSkipWhitespace<InsituStringStream>();
}

TEST(SIMD, SIMD_SUFFIX(SkipWhitespace_EncodedMemoryStream)) {
    for (size_t step = 1; step < 32; step++) {
        char buffer[SIMD_SIZE_ALIGN(1024)];
        for (size_t i = 0; i < 1024; i++)
            buffer[i] = " \t\r\n"[i % 4];
        for (size_t i = 0; i < 1024; i += step)
            buffer[i] = 'X';

        MemoryStream ms(buffer, 1024);
        EncodedInputStream<UTF8<>, MemoryStream> s(ms);
        for (;;) {
            SkipWhitespace(s);
            if (s.peek() == '\0')
                break;
            //EXPECT_EQ(i, s.tellg());
            EXPECT_EQ('X', s.get());
        }
    }
}

struct ScanCopyUnescapedStringHandler : BaseReaderHandler<UTF8<>, ScanCopyUnescapedStringHandler> {
    bool emplace_string(const char* str, size_t length, bool) {
        memcpy(buffer, str, length + 1);
        return true;
    }
    char buffer[1024 + 5 + 32];
};

template <unsigned parseFlags, typename StreamType>
void TestScanCopyUnescapedString() {
    char buffer[SIMD_SIZE_ALIGN(1024u + 5 + 32)];
    char backup[SIMD_SIZE_ALIGN(1024u + 5 + 32)];

    // Test "ABCDABCD...\\"
    for (size_t offset = 0; offset < 32; offset++) {
        for (size_t step = 0; step < 1024; step++) {
            char* json = buffer + offset;
            char *p = json;
            *p++ = '\"';
            for (size_t i = 0; i < step; i++)
                *p++ = "ABCD"[i % 4];
            *p++ = '\\';
            *p++ = '\\';
            *p++ = '\"';
            *p++ = '\0';
            strcpy(backup, json); // insitu parsing will overwrite buffer, so need to backup first

            StreamType s(json);
            Reader reader;
            ScanCopyUnescapedStringHandler h;
            reader.parse<parseFlags>(s, h);
            EXPECT_TRUE(memcmp(h.buffer, backup + 1, step) == 0);
            EXPECT_EQ('\\', h.buffer[step]);    // escaped
            EXPECT_EQ('\0', h.buffer[step + 1]);
        }
    }

    // Test "\\ABCDABCD..."
    for (size_t offset = 0; offset < 32; offset++) {
        for (size_t step = 0; step < 1024; step++) {
            char* json = buffer + offset;
            char *p = json;
            *p++ = '\"';
            *p++ = '\\';
            *p++ = '\\';
            for (size_t i = 0; i < step; i++)
                *p++ = "ABCD"[i % 4];
            *p++ = '\"';
            *p++ = '\0';
            strcpy(backup, json); // insitu parsing will overwrite buffer, so need to backup first

            StreamType s(json);
            Reader reader;
            ScanCopyUnescapedStringHandler h;
            reader.parse<parseFlags>(s, h);
            EXPECT_TRUE(memcmp(h.buffer + 1, backup + 3, step) == 0);
            EXPECT_EQ('\\', h.buffer[0]);    // escaped
            EXPECT_EQ('\0', h.buffer[step + 1]);
        }
    }
}

TEST(SIMD, SIMD_SUFFIX(ScanCopyUnescapedString)) {
    TestScanCopyUnescapedString<kParseDefaultFlags, StringStream>();
}

TEST(SIMD, SIMD_SUFFIX(ScanWriteUnescapedString)) {
    char buffer[SIMD_SIZE_ALIGN(2048 + 1 + 32)];
    for (size_t offset = 0; offset < 32; offset++) {
        for (size_t step = 0; step < 1024; step++) {
            char* s = buffer + offset;
            char* p = s;
            for (size_t i = 0; i < step; i++)
                *p++ = "ABCD"[i % 4];
            char escape = "\0\n\\\""[step % 4];
            *p++ = escape;
            for (size_t i = 0; i < step; i++)
                *p++ = "ABCD"[i % 4];

            StringBuffer sb;
            Writer<StringBuffer> writer(sb);
            writer.emplace_string(s, SizeType(step * 2 + 1));
            const char* q = sb.get_string();
            EXPECT_EQ('\"', *q++);
            for (size_t i = 0; i < step; i++)
                EXPECT_EQ("ABCD"[i % 4], *q++);
            if (escape == '\0') {
                EXPECT_EQ('\\', *q++);
                EXPECT_EQ('u', *q++);
                EXPECT_EQ('0', *q++);
                EXPECT_EQ('0', *q++);
                EXPECT_EQ('0', *q++);
                EXPECT_EQ('0', *q++);
            }
            else if (escape == '\n') {
                EXPECT_EQ('\\', *q++);
                EXPECT_EQ('n', *q++);
            }
            else if (escape == '\\') {
                EXPECT_EQ('\\', *q++);
                EXPECT_EQ('\\', *q++);
            }
            else if (escape == '\"') {
                EXPECT_EQ('\\', *q++);
                EXPECT_EQ('\"', *q++);
            }
            for (size_t i = 0; i < step; i++)
                EXPECT_EQ("ABCD"[i % 4], *q++);
            EXPECT_EQ('\"', *q++);
            EXPECT_EQ('\0', *q++);
        }
    }
}

#ifdef __GNUC__
MERAK_JSON_DIAG_POP
#endif
