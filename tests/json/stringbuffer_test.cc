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
#include <merak/json/stringbuffer.h>
#include <merak/json/writer.h>

#ifdef __clang__
MERAK_JSON_DIAG_PUSH
MERAK_JSON_DIAG_OFF(c++98-compat)
#endif

using namespace merak::json;

TEST(StringBuffer, InitialSize) {
    StringBuffer buffer;
    EXPECT_EQ(0u, buffer.GetSize());
    EXPECT_EQ(0u, buffer.get_length());
    EXPECT_STREQ("", buffer.get_string());
}

TEST(StringBuffer, Put) {
    StringBuffer buffer;
    buffer.put('A');

    EXPECT_EQ(1u, buffer.GetSize());
    EXPECT_EQ(1u, buffer.get_length());
    EXPECT_STREQ("A", buffer.get_string());
}

TEST(StringBuffer, PutN_Issue672) {
    GenericStringBuffer<UTF8<>, MemoryPoolAllocator<> > buffer;
    EXPECT_EQ(0u, buffer.GetSize());
    EXPECT_EQ(0u, buffer.get_length());
    merak::json::PutN(buffer, ' ', 1);
    EXPECT_EQ(1u, buffer.GetSize());
    EXPECT_EQ(1u, buffer.get_length());
}

TEST(StringBuffer, clear) {
    StringBuffer buffer;
    buffer.put('A');
    buffer.put('B');
    buffer.put('C');
    buffer.clear();

    EXPECT_EQ(0u, buffer.GetSize());
    EXPECT_EQ(0u, buffer.get_length());
    EXPECT_STREQ("", buffer.get_string());
}

TEST(StringBuffer, Push) {
    StringBuffer buffer;
    buffer.Push(5);

    EXPECT_EQ(5u, buffer.GetSize());
    EXPECT_EQ(5u, buffer.get_length());

    // Causes sudden expansion to make the stack's capacity equal to size
    buffer.Push(65536u);
    EXPECT_EQ(5u + 65536u, buffer.GetSize());
}

TEST(StringBuffer, Pop) {
    StringBuffer buffer;
    buffer.put('A');
    buffer.put('B');
    buffer.put('C');
    buffer.put('D');
    buffer.put('E');
    buffer.Pop(3);

    EXPECT_EQ(2u, buffer.GetSize());
    EXPECT_EQ(2u, buffer.get_length());
    EXPECT_STREQ("AB", buffer.get_string());
}

TEST(StringBuffer, GetLength_Issue744) {
    GenericStringBuffer<UTF16<wchar_t> > buffer;
    buffer.put('A');
    buffer.put('B');
    buffer.put('C');
    EXPECT_EQ(3u * sizeof(wchar_t), buffer.GetSize());
    EXPECT_EQ(3u, buffer.get_length());
}


#if 0 // Many old compiler does not support these. Turn it off temporaily.

#include <type_traits>

TEST(StringBuffer, Traits) {
    static_assert( std::is_constructible<StringBuffer>::value, "");
    static_assert( std::is_default_constructible<StringBuffer>::value, "");
#ifndef _MSC_VER
    static_assert(!std::is_copy_constructible<StringBuffer>::value, "");
#endif
    static_assert( std::is_move_constructible<StringBuffer>::value, "");

    static_assert(!std::is_nothrow_constructible<StringBuffer>::value, "");
    static_assert(!std::is_nothrow_default_constructible<StringBuffer>::value, "");

#if !defined(_MSC_VER) || _MSC_VER >= 1800
    static_assert(!std::is_nothrow_copy_constructible<StringBuffer>::value, "");
    static_assert(!std::is_nothrow_move_constructible<StringBuffer>::value, "");
#endif

    static_assert( std::is_assignable<StringBuffer,StringBuffer>::value, "");
#ifndef _MSC_VER
    static_assert(!std::is_copy_assignable<StringBuffer>::value, "");
#endif
    static_assert( std::is_move_assignable<StringBuffer>::value, "");

#if !defined(_MSC_VER) || _MSC_VER >= 1800
    static_assert(!std::is_nothrow_assignable<StringBuffer, StringBuffer>::value, "");
#endif

    static_assert(!std::is_nothrow_copy_assignable<StringBuffer>::value, "");
    static_assert(!std::is_nothrow_move_assignable<StringBuffer>::value, "");

    static_assert( std::is_destructible<StringBuffer>::value, "");
#ifndef _MSC_VER
    static_assert(std::is_nothrow_destructible<StringBuffer>::value, "");
#endif
}

#endif

TEST(StringBuffer, MoveConstructor) {
    StringBuffer x;
    x.put('A');
    x.put('B');
    x.put('C');
    x.put('D');

    EXPECT_EQ(4u, x.GetSize());
    EXPECT_EQ(4u, x.get_length());
    EXPECT_STREQ("ABCD", x.get_string());

    // StringBuffer y(x); // does not compile (!is_copy_constructible)
    StringBuffer y(std::move(x));
    EXPECT_EQ(0u, x.GetSize());
    EXPECT_EQ(0u, x.get_length());
    EXPECT_EQ(4u, y.GetSize());
    EXPECT_EQ(4u, y.get_length());
    EXPECT_STREQ("ABCD", y.get_string());

    // StringBuffer z = y; // does not compile (!is_copy_assignable)
    StringBuffer z = std::move(y);
    EXPECT_EQ(0u, y.GetSize());
    EXPECT_EQ(0u, y.get_length());
    EXPECT_EQ(4u, z.GetSize());
    EXPECT_EQ(4u, z.get_length());
    EXPECT_STREQ("ABCD", z.get_string());
}

TEST(StringBuffer, MoveAssignment) {
    StringBuffer x;
    x.put('A');
    x.put('B');
    x.put('C');
    x.put('D');

    EXPECT_EQ(4u, x.GetSize());
    EXPECT_EQ(4u, x.get_length());
    EXPECT_STREQ("ABCD", x.get_string());

    StringBuffer y;
    // y = x; // does not compile (!is_copy_assignable)
    y = std::move(x);
    EXPECT_EQ(0u, x.GetSize());
    EXPECT_EQ(4u, y.get_length());
    EXPECT_STREQ("ABCD", y.get_string());
}


#ifdef __clang__
MERAK_JSON_DIAG_POP
#endif
