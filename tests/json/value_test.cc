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
#include <merak/json/std_output_stream.h>
#include <algorithm>

#ifdef __clang__
MERAK_JSON_DIAG_PUSH
MERAK_JSON_DIAG_OFF(c++98-compat)
#endif

using namespace merak::json;

TEST(Value, size) {
    if (sizeof(SizeType) == 4) {
#if MERAK_JSON_48BITPOINTER_OPTIMIZATION
        EXPECT_EQ(16u, sizeof(Value));
#elif MERAK_JSON_64BIT
        EXPECT_EQ(24u, sizeof(Value));
#else
        EXPECT_EQ(16u, sizeof(Value));
#endif
    }
}

TEST(Value, DefaultConstructor) {
    Value x;
    EXPECT_EQ(kNullType, x.get_type());
    EXPECT_TRUE(x.is_null());

    //std::cout << "sizeof(Value): " << sizeof(x) << std::endl;
}

// Should not pass compilation
//TEST(Value, copy_constructor) {
//  Value x(1234);
//  Value y = x;
//}


#if 0 // Many old compiler does not support these. Turn it off temporaily.

#include <type_traits>

TEST(Value, Traits) {
    typedef GenericValue<UTF8<>, CrtAllocator> Value;
    static_assert(std::is_constructible<Value>::value, "");
    static_assert(std::is_default_constructible<Value>::value, "");
#ifndef _MSC_VER
    static_assert(!std::is_copy_constructible<Value>::value, "");
#endif
    static_assert(std::is_move_constructible<Value>::value, "");

#ifndef _MSC_VER
    static_assert(std::is_nothrow_constructible<Value>::value, "");
    static_assert(std::is_nothrow_default_constructible<Value>::value, "");
    static_assert(!std::is_nothrow_copy_constructible<Value>::value, "");
    static_assert(std::is_nothrow_move_constructible<Value>::value, "");
#endif

    static_assert(std::is_assignable<Value,Value>::value, "");
#ifndef _MSC_VER
    static_assert(!std::is_copy_assignable<Value>::value, "");
#endif
    static_assert(std::is_move_assignable<Value>::value, "");

#ifndef _MSC_VER
    static_assert(std::is_nothrow_assignable<Value, Value>::value, "");
#endif
    static_assert(!std::is_nothrow_copy_assignable<Value>::value, "");
#ifndef _MSC_VER
    static_assert(std::is_nothrow_move_assignable<Value>::value, "");
#endif

    static_assert(std::is_destructible<Value>::value, "");
#ifndef _MSC_VER
    static_assert(std::is_nothrow_destructible<Value>::value, "");
#endif
}

#endif

TEST(Value, MoveConstructor) {
    typedef GenericValue<UTF8<>, CrtAllocator> V;
    V::AllocatorType allocator;

    V x((V(kArrayType)));
    x.reserve(4u, allocator);
    x.push_back(1, allocator).push_back(2, allocator).push_back(3, allocator).push_back(4, allocator);
    EXPECT_TRUE(x.is_array());
    EXPECT_EQ(4u, x.size());

    // Value y(x); // does not compile (!is_copy_constructible)
    V y(std::move(x));
    EXPECT_TRUE(x.is_null());
    EXPECT_TRUE(y.is_array());
    EXPECT_EQ(4u, y.size());

    // Value z = y; // does not compile (!is_copy_assignable)
    V z = std::move(y);
    EXPECT_TRUE(y.is_null());
    EXPECT_TRUE(z.is_array());
    EXPECT_EQ(4u, z.size());
}


TEST(Value, AssignmentOperator) {
    Value x(1234);
    Value y;
    y = x;
    EXPECT_TRUE(x.is_null());    // move semantic
    EXPECT_EQ(1234, y.get_int32());

    y = 5678;
    EXPECT_TRUE(y.is_int32());
    EXPECT_EQ(5678, y.get_int32());

    x = "Hello";
    EXPECT_TRUE(x.is_string());
    EXPECT_STREQ(x.get_string(),"Hello");

    y = StringRef(x.get_string(),x.get_string_length());
    EXPECT_TRUE(y.is_string());
    EXPECT_EQ(y.get_string(),x.get_string());
    EXPECT_EQ(y.get_string_length(),x.get_string_length());

    static char mstr[] = "mutable";
    // y = mstr; // should not compile
    y = StringRef(mstr);
    EXPECT_TRUE(y.is_string());
    EXPECT_EQ(y.get_string(),mstr);

    // C++11 move assignment
    x = Value("World");
    EXPECT_TRUE(x.is_string());
    EXPECT_STREQ("World", x.get_string());

    x = std::move(y);
    EXPECT_TRUE(y.is_null());
    EXPECT_TRUE(x.is_string());
    EXPECT_EQ(x.get_string(), mstr);

    y = std::move(Value().set_int32(1234));
    EXPECT_TRUE(y.is_int32());
    EXPECT_EQ(1234, y);
}

template <typename A, typename B> 
void TestEqual(const A& a, const B& b) {
    EXPECT_TRUE (a == b);
    EXPECT_FALSE(a != b);
    EXPECT_TRUE (b == a);
    EXPECT_FALSE(b != a);
}

template <typename A, typename B> 
void TestUnequal(const A& a, const B& b) {
    EXPECT_FALSE(a == b);
    EXPECT_TRUE (a != b);
    EXPECT_FALSE(b == a);
    EXPECT_TRUE (b != a);
}

TEST(Value, EqualtoOperator) {
    Value::AllocatorType allocator;
    Value x(kObjectType);
    x.add_member("hello", "world", allocator)
        .add_member("t", Value(true).Move(), allocator)
        .add_member("f", Value(false).Move(), allocator)
        .add_member("n", Value(kNullType).Move(), allocator)
        .add_member("i", 123, allocator)
        .add_member("pi", 3.14, allocator)
        .add_member("a", Value(kArrayType).Move().push_back(1, allocator).push_back(2, allocator).push_back(3, allocator), allocator);

    // Test templated operator==() and operator!=()
    TestEqual(x["hello"], "world");
    const char* cc = "world";
    TestEqual(x["hello"], cc);
    char* c = strdup("world");
    TestEqual(x["hello"], c);
    free(c);

    TestEqual(x["t"], true);
    TestEqual(x["f"], false);
    TestEqual(x["i"], 123);
    TestEqual(x["pi"], 3.14);

    // Test operator==() (including different allocators)
    CrtAllocator crtAllocator;
    GenericValue<UTF8<>, CrtAllocator> y;
    GenericDocument<UTF8<>, CrtAllocator> z(&crtAllocator);
    y.copy_from(x, crtAllocator);
    z.copy_from(y, z.get_allocator());
    TestEqual(x, y);
    TestEqual(y, z);
    TestEqual(z, x);

    // Swapping member order should be fine.
    EXPECT_TRUE(y.remove_member("t"));
    TestUnequal(x, y);
    TestUnequal(z, y);
    EXPECT_TRUE(z.remove_member("t"));
    TestUnequal(x, z);
    TestEqual(y, z);
    y.add_member("t", false, crtAllocator);
    z.add_member("t", false, z.get_allocator());
    TestUnequal(x, y);
    TestUnequal(z, x);
    y["t"] = true;
    z["t"] = true;
    TestEqual(x, y);
    TestEqual(y, z);
    TestEqual(z, x);

    // Swapping element order is not OK
    x["a"][0].swap(x["a"][1]);
    TestUnequal(x, y);
    x["a"][0].swap(x["a"][1]);
    TestEqual(x, y);

    // Array of different size
    x["a"].push_back(4, allocator);
    TestUnequal(x, y);
    x["a"].pop_back();
    TestEqual(x, y);

    // Issue #129: compare Uint64
    x.set_uint64(MERAK_JSON_UINT64_C2(0xFFFFFFFF, 0xFFFFFFF0));
    y.set_uint64(MERAK_JSON_UINT64_C2(0xFFFFFFFF, 0xFFFFFFFF));
    TestUnequal(x, y);
}

template <typename Value>
void TestCopyFrom() {
    typename Value::AllocatorType a;
    Value v1(1234);
    Value v2(v1, a); // deep copy constructor
    EXPECT_TRUE(v1.get_type() == v2.get_type());
    EXPECT_EQ(v1.get_int32(), v2.get_int32());

    v1.set_string("foo");
    v2.copy_from(v1, a);
    EXPECT_TRUE(v1.get_type() == v2.get_type());
    EXPECT_STREQ(v1.get_string(), v2.get_string());
    EXPECT_EQ(v1.get_string(), v2.get_string()); // string NOT copied

    v1.set_string("bar", a); // copy string
    v2.copy_from(v1, a);
    EXPECT_TRUE(v1.get_type() == v2.get_type());
    EXPECT_STREQ(v1.get_string(), v2.get_string());
    EXPECT_NE(v1.get_string(), v2.get_string()); // string copied


    v1.set_array().push_back(1234, a);
    v2.copy_from(v1, a);
    EXPECT_TRUE(v2.is_array());
    EXPECT_EQ(v1.size(), v2.size());

    v1.push_back(Value().set_string("foo", a), a); // push string copy
    EXPECT_TRUE(v1.size() != v2.size());
    v2.copy_from(v1, a);
    EXPECT_TRUE(v1.size() == v2.size());
    EXPECT_STREQ(v1[1].get_string(), v2[1].get_string());
    EXPECT_NE(v1[1].get_string(), v2[1].get_string()); // string got copied
}

TEST(Value, copy_from) {
    TestCopyFrom<Value>();
    TestCopyFrom<GenericValue<UTF8<>, CrtAllocator> >();
}

TEST(Value, swap) {
    Value v1(1234);
    Value v2(kObjectType);

    EXPECT_EQ(&v1, &v1.swap(v2));
    EXPECT_TRUE(v1.is_object());
    EXPECT_TRUE(v2.is_int32());
    EXPECT_EQ(1234, v2.get_int32());

    // testing std::swap compatibility
    using std::swap;
    swap(v1, v2);
    EXPECT_TRUE(v1.is_int32());
    EXPECT_TRUE(v2.is_object());
}

TEST(Value, Null) {
    // Default constructor
    Value x;
    EXPECT_EQ(kNullType, x.get_type());
    EXPECT_TRUE(x.is_null());

    EXPECT_FALSE(x.is_true());
    EXPECT_FALSE(x.is_false());
    EXPECT_FALSE(x.is_number());
    EXPECT_FALSE(x.is_string());
    EXPECT_FALSE(x.is_object());
    EXPECT_FALSE(x.is_array());

    // Constructor with type
    Value y(kNullType);
    EXPECT_TRUE(y.is_null());

    // SetNull();
    Value z(true);
    z.SetNull();
    EXPECT_TRUE(z.is_null());
}

TEST(Value, True) {
    // Constructor with bool
    Value x(true);
    EXPECT_EQ(kTrueType, x.get_type());
    EXPECT_TRUE(x.get_bool());
    EXPECT_TRUE(x.is_bool());
    EXPECT_TRUE(x.is_true());

    EXPECT_FALSE(x.is_null());
    EXPECT_FALSE(x.is_false());
    EXPECT_FALSE(x.is_number());
    EXPECT_FALSE(x.is_string());
    EXPECT_FALSE(x.is_object());
    EXPECT_FALSE(x.is_array());

    // Constructor with type
    Value y(kTrueType);
    EXPECT_TRUE(y.is_true());

    // SetBool()
    Value z;
    z.SetBool(true);
    EXPECT_TRUE(z.is_true());

    // Templated functions
    EXPECT_TRUE(z.is_type<bool>());
    EXPECT_TRUE(z.get<bool>());
    EXPECT_FALSE(z.set<bool>(false).get<bool>());
    EXPECT_TRUE(z.set(true).get<bool>());
}

TEST(Value, False) {
    // Constructor with bool
    Value x(false);
    EXPECT_EQ(kFalseType, x.get_type());
    EXPECT_TRUE(x.is_bool());
    EXPECT_TRUE(x.is_false());

    EXPECT_FALSE(x.is_null());
    EXPECT_FALSE(x.is_true());
    EXPECT_FALSE(x.get_bool());
    //EXPECT_FALSE((bool)x);
    EXPECT_FALSE(x.is_number());
    EXPECT_FALSE(x.is_string());
    EXPECT_FALSE(x.is_object());
    EXPECT_FALSE(x.is_array());

    // Constructor with type
    Value y(kFalseType);
    EXPECT_TRUE(y.is_false());

    // SetBool()
    Value z;
    z.SetBool(false);
    EXPECT_TRUE(z.is_false());
}

TEST(Value, Int) {
    // Constructor with int
    Value x(1234);
    EXPECT_EQ(kNumberType, x.get_type());
    EXPECT_EQ(1234, x.get_int32());
    EXPECT_EQ(1234u, x.get_uint32());
    EXPECT_EQ(1234, x.get_int64());
    EXPECT_EQ(1234u, x.get_uint64());
    EXPECT_NEAR(1234.0, x.get_double(), 0.0);
    //EXPECT_EQ(1234, (int)x);
    //EXPECT_EQ(1234, (unsigned)x);
    //EXPECT_EQ(1234, (int64_t)x);
    //EXPECT_EQ(1234, (uint64_t)x);
    //EXPECT_EQ(1234, (double)x);
    EXPECT_TRUE(x.is_number());
    EXPECT_TRUE(x.is_int32());
    EXPECT_TRUE(x.is_uint32());
    EXPECT_TRUE(x.is_int64());
    EXPECT_TRUE(x.is_uint64());

    EXPECT_FALSE(x.is_double());
    EXPECT_FALSE(x.is_float());
    EXPECT_FALSE(x.is_null());
    EXPECT_FALSE(x.is_bool());
    EXPECT_FALSE(x.is_false());
    EXPECT_FALSE(x.is_true());
    EXPECT_FALSE(x.is_string());
    EXPECT_FALSE(x.is_object());
    EXPECT_FALSE(x.is_array());

    Value nx(-1234);
    EXPECT_EQ(-1234, nx.get_int32());
    EXPECT_EQ(-1234, nx.get_int64());
    EXPECT_TRUE(nx.is_int32());
    EXPECT_TRUE(nx.is_int64());
    EXPECT_FALSE(nx.is_uint32());
    EXPECT_FALSE(nx.is_uint64());

    // Constructor with type
    Value y(kNumberType);
    EXPECT_TRUE(y.is_number());
    EXPECT_TRUE(y.is_int32());
    EXPECT_EQ(0, y.get_int32());

    // set_int32()
    Value z;
    z.set_int32(1234);
    EXPECT_EQ(1234, z.get_int32());

    // operator=(int)
    z = 5678;
    EXPECT_EQ(5678, z.get_int32());

    // Templated functions
    EXPECT_TRUE(z.is_type<int>());
    EXPECT_EQ(5678, z.get<int>());
    EXPECT_EQ(5679, z.set(5679).get<int>());
    EXPECT_EQ(5680, z.set<int>(5680).get<int>());

#ifdef _MSC_VER
    // long as int on MSC platforms
    MERAK_JSON_STATIC_ASSERT(sizeof(long) == sizeof(int));
    z.set_int32(2222);
    EXPECT_TRUE(z.is_type<long>());
    EXPECT_EQ(2222l, z.Get<long>());
    EXPECT_EQ(3333l, z.set(3333l).Get<long>());
    EXPECT_EQ(4444l, z.set<long>(4444l).Get<long>());
    EXPECT_TRUE(z.is_int32());
#endif
}

TEST(Value, Uint) {
    // Constructor with int
    Value x(1234u);
    EXPECT_EQ(kNumberType, x.get_type());
    EXPECT_EQ(1234, x.get_int32());
    EXPECT_EQ(1234u, x.get_uint32());
    EXPECT_EQ(1234, x.get_int64());
    EXPECT_EQ(1234u, x.get_uint64());
    EXPECT_TRUE(x.is_number());
    EXPECT_TRUE(x.is_int32());
    EXPECT_TRUE(x.is_uint32());
    EXPECT_TRUE(x.is_int64());
    EXPECT_TRUE(x.is_uint64());
    EXPECT_NEAR(1234.0, x.get_double(), 0.0);   // Number can always be cast as double but !is_double().

    EXPECT_FALSE(x.is_double());
    EXPECT_FALSE(x.is_float());
    EXPECT_FALSE(x.is_null());
    EXPECT_FALSE(x.is_bool());
    EXPECT_FALSE(x.is_false());
    EXPECT_FALSE(x.is_true());
    EXPECT_FALSE(x.is_string());
    EXPECT_FALSE(x.is_object());
    EXPECT_FALSE(x.is_array());

    // set_uint32()
    Value z;
    z.set_uint32(1234);
    EXPECT_EQ(1234u, z.get_uint32());

    // operator=(unsigned)
    z = 5678u;
    EXPECT_EQ(5678u, z.get_uint32());

    z = 2147483648u;    // 2^31, cannot cast as int
    EXPECT_EQ(2147483648u, z.get_uint32());
    EXPECT_FALSE(z.is_int32());
    EXPECT_TRUE(z.is_int64());   // Issue 41: Incorrect parsing of unsigned int number types

    // Templated functions
    EXPECT_TRUE(z.is_type<unsigned>());
    EXPECT_EQ(2147483648u, z.get<unsigned>());
    EXPECT_EQ(2147483649u, z.set(2147483649u).get<unsigned>());
    EXPECT_EQ(2147483650u, z.set<unsigned>(2147483650u).get<unsigned>());

#ifdef _MSC_VER
    // unsigned long as unsigned on MSC platforms
    MERAK_JSON_STATIC_ASSERT(sizeof(unsigned long) == sizeof(unsigned));
    z.set_uint32(2222);
    EXPECT_TRUE(z.is_type<unsigned long>());
    EXPECT_EQ(2222ul, z.get<unsigned long>());
    EXPECT_EQ(3333ul, z.set(3333ul).Get<unsigned long>());
    EXPECT_EQ(4444ul, z.set<unsigned long>(4444ul).Get<unsigned long>());
    EXPECT_TRUE(x.is_uint32());
#endif
}

TEST(Value, Int64) {
    // Constructor with int
    Value x(int64_t(1234));
    EXPECT_EQ(kNumberType, x.get_type());
    EXPECT_EQ(1234, x.get_int32());
    EXPECT_EQ(1234u, x.get_uint32());
    EXPECT_EQ(1234, x.get_int64());
    EXPECT_EQ(1234u, x.get_uint64());
    EXPECT_TRUE(x.is_number());
    EXPECT_TRUE(x.is_int32());
    EXPECT_TRUE(x.is_uint32());
    EXPECT_TRUE(x.is_int64());
    EXPECT_TRUE(x.is_uint64());

    EXPECT_FALSE(x.is_double());
    EXPECT_FALSE(x.is_float());
    EXPECT_FALSE(x.is_null());
    EXPECT_FALSE(x.is_bool());
    EXPECT_FALSE(x.is_false());
    EXPECT_FALSE(x.is_true());
    EXPECT_FALSE(x.is_string());
    EXPECT_FALSE(x.is_object());
    EXPECT_FALSE(x.is_array());

    Value nx(int64_t(-1234));
    EXPECT_EQ(-1234, nx.get_int32());
    EXPECT_EQ(-1234, nx.get_int64());
    EXPECT_TRUE(nx.is_int32());
    EXPECT_TRUE(nx.is_int64());
    EXPECT_FALSE(nx.is_uint32());
    EXPECT_FALSE(nx.is_uint64());

    // set_int64()
    Value z;
    z.set_int64(1234);
    EXPECT_EQ(1234, z.get_int64());

    z.set_int64(2147483648u);   // 2^31, cannot cast as int
    EXPECT_FALSE(z.is_int32());
    EXPECT_TRUE(z.is_uint32());
    EXPECT_NEAR(2147483648.0, z.get_double(), 0.0);

    z.set_int64(int64_t(4294967295u) + 1);   // 2^32, cannot cast as uint
    EXPECT_FALSE(z.is_int32());
    EXPECT_FALSE(z.is_uint32());
    EXPECT_NEAR(4294967296.0, z.get_double(), 0.0);

    z.set_int64(-int64_t(2147483648u) - 1);   // -2^31-1, cannot cast as int
    EXPECT_FALSE(z.is_int32());
    EXPECT_NEAR(-2147483649.0, z.get_double(), 0.0);

    int64_t i = static_cast<int64_t>(MERAK_JSON_UINT64_C2(0x80000000, 00000000));
    z.set_int64(i);
    EXPECT_DOUBLE_EQ(-9223372036854775808.0, z.get_double());

    // Templated functions
    EXPECT_TRUE(z.is_type<int64_t>());
    EXPECT_EQ(i, z.get<int64_t>());
#if 0 // signed integer underflow is undefined behaviour
    EXPECT_EQ(i - 1, z.set(i - 1).Get<int64_t>());
    EXPECT_EQ(i - 2, z.set<int64_t>(i - 2).Get<int64_t>());
#endif
}

TEST(Value, Uint64) {
    // Constructor with int
    Value x(uint64_t(1234));
    EXPECT_EQ(kNumberType, x.get_type());
    EXPECT_EQ(1234, x.get_int32());
    EXPECT_EQ(1234u, x.get_uint32());
    EXPECT_EQ(1234, x.get_int64());
    EXPECT_EQ(1234u, x.get_uint64());
    EXPECT_TRUE(x.is_number());
    EXPECT_TRUE(x.is_int32());
    EXPECT_TRUE(x.is_uint32());
    EXPECT_TRUE(x.is_int64());
    EXPECT_TRUE(x.is_uint64());

    EXPECT_FALSE(x.is_double());
    EXPECT_FALSE(x.is_float());
    EXPECT_FALSE(x.is_null());
    EXPECT_FALSE(x.is_bool());
    EXPECT_FALSE(x.is_false());
    EXPECT_FALSE(x.is_true());
    EXPECT_FALSE(x.is_string());
    EXPECT_FALSE(x.is_object());
    EXPECT_FALSE(x.is_array());

    // set_uint64()
    Value z;
    z.set_uint64(1234);
    EXPECT_EQ(1234u, z.get_uint64());

    z.set_uint64(uint64_t(2147483648u));  // 2^31, cannot cast as int
    EXPECT_FALSE(z.is_int32());
    EXPECT_TRUE(z.is_uint32());
    EXPECT_TRUE(z.is_int64());

    z.set_uint64(uint64_t(4294967295u) + 1);  // 2^32, cannot cast as uint
    EXPECT_FALSE(z.is_int32());
    EXPECT_FALSE(z.is_uint32());
    EXPECT_TRUE(z.is_int64());

    uint64_t u = MERAK_JSON_UINT64_C2(0x80000000, 0x00000000);
    z.set_uint64(u);    // 2^63 cannot cast as int64
    EXPECT_FALSE(z.is_int64());
    EXPECT_EQ(u, z.get_uint64()); // Issue 48
    EXPECT_DOUBLE_EQ(9223372036854775808.0, z.get_double());

    // Templated functions
    EXPECT_TRUE(z.is_type<uint64_t>());
    EXPECT_EQ(u, z.get<uint64_t>());
    EXPECT_EQ(u + 1, z.set(u + 1).get<uint64_t>());
    EXPECT_EQ(u + 2, z.set<uint64_t>(u + 2).get<uint64_t>());
}

TEST(Value, Double) {
    // Constructor with double
    Value x(12.34);
    EXPECT_EQ(kNumberType, x.get_type());
    EXPECT_NEAR(12.34, x.get_double(), 0.0);
    EXPECT_TRUE(x.is_number());
    EXPECT_TRUE(x.is_double());

    EXPECT_FALSE(x.is_int32());
    EXPECT_FALSE(x.is_null());
    EXPECT_FALSE(x.is_bool());
    EXPECT_FALSE(x.is_false());
    EXPECT_FALSE(x.is_true());
    EXPECT_FALSE(x.is_string());
    EXPECT_FALSE(x.is_object());
    EXPECT_FALSE(x.is_array());

    // set_double()
    Value z;
    z.set_double(12.34);
    EXPECT_NEAR(12.34, z.get_double(), 0.0);

    z = 56.78;
    EXPECT_NEAR(56.78, z.get_double(), 0.0);

    // Templated functions
    EXPECT_TRUE(z.is_type<double>());
    EXPECT_EQ(56.78, z.get<double>());
    EXPECT_EQ(57.78, z.set(57.78).get<double>());
    EXPECT_EQ(58.78, z.set<double>(58.78).get<double>());
}

TEST(Value, Float) {
    // Constructor with double
    Value x(12.34f);
    EXPECT_EQ(kNumberType, x.get_type());
    EXPECT_NEAR(12.34f, x.get_float(), 0.0);
    EXPECT_TRUE(x.is_number());
    EXPECT_TRUE(x.is_double());
    EXPECT_TRUE(x.is_float());

    EXPECT_FALSE(x.is_int32());
    EXPECT_FALSE(x.is_null());
    EXPECT_FALSE(x.is_bool());
    EXPECT_FALSE(x.is_false());
    EXPECT_FALSE(x.is_true());
    EXPECT_FALSE(x.is_string());
    EXPECT_FALSE(x.is_object());
    EXPECT_FALSE(x.is_array());

    // set_float()
    Value z;
    z.set_float(12.34f);
    EXPECT_NEAR(12.34f, z.get_float(), 0.0f);

    // Issue 573
    z.set_int32(0);
    EXPECT_EQ(0.0f, z.get_float());

    z = 56.78f;
    EXPECT_NEAR(56.78f, z.get_float(), 0.0f);

    // Templated functions
    EXPECT_TRUE(z.is_type<float>());
    EXPECT_EQ(56.78f, z.get<float>());
    EXPECT_EQ(57.78f, z.set(57.78f).get<float>());
    EXPECT_EQ(58.78f, z.set<float>(58.78f).get<float>());
}

TEST(Value, is_lossless_double) {
    EXPECT_TRUE(Value(0.0).is_lossless_double());
    EXPECT_TRUE(Value(12.34).is_lossless_double());
    EXPECT_TRUE(Value(-123).is_lossless_double());
    EXPECT_TRUE(Value(2147483648u).is_lossless_double());
    EXPECT_TRUE(Value(-static_cast<int64_t>(MERAK_JSON_UINT64_C2(0x40000000, 0x00000000))).is_lossless_double());
#if !(defined(_MSC_VER) && _MSC_VER < 1800) // VC2010 has problem
    EXPECT_TRUE(Value(MERAK_JSON_UINT64_C2(0xA0000000, 0x00000000)).is_lossless_double());
#endif

    EXPECT_FALSE(Value(static_cast<int64_t>(MERAK_JSON_UINT64_C2(0x7FFFFFFF, 0xFFFFFFFF))).is_lossless_double()); // INT64_MAX
    EXPECT_FALSE(Value(-static_cast<int64_t>(MERAK_JSON_UINT64_C2(0x7FFFFFFF, 0xFFFFFFFF))).is_lossless_double()); // -INT64_MAX
    EXPECT_TRUE(Value(-static_cast<int64_t>(MERAK_JSON_UINT64_C2(0x7FFFFFFF, 0xFFFFFFFF)) - 1).is_lossless_double()); // INT64_MIN
    EXPECT_FALSE(Value(MERAK_JSON_UINT64_C2(0xFFFFFFFF, 0xFFFFFFFF)).is_lossless_double()); // UINT64_MAX

    EXPECT_TRUE(Value(3.4028234e38f).is_lossless_double()); // FLT_MAX
    EXPECT_TRUE(Value(-3.4028234e38f).is_lossless_double()); // -FLT_MAX
    EXPECT_TRUE(Value(1.17549435e-38f).is_lossless_double()); // FLT_MIN
    EXPECT_TRUE(Value(-1.17549435e-38f).is_lossless_double()); // -FLT_MIN
    EXPECT_TRUE(Value(1.7976931348623157e+308).is_lossless_double()); // DBL_MAX
    EXPECT_TRUE(Value(-1.7976931348623157e+308).is_lossless_double()); // -DBL_MAX
    EXPECT_TRUE(Value(2.2250738585072014e-308).is_lossless_double()); // DBL_MIN
    EXPECT_TRUE(Value(-2.2250738585072014e-308).is_lossless_double()); // -DBL_MIN
}

TEST(Value, is_lossless_float) {
    EXPECT_TRUE(Value(12.25).is_lossless_float());
    EXPECT_TRUE(Value(-123).is_lossless_float());
    EXPECT_TRUE(Value(2147483648u).is_lossless_float());
    EXPECT_TRUE(Value(3.4028234e38f).is_lossless_float());
    EXPECT_TRUE(Value(-3.4028234e38f).is_lossless_float());
    EXPECT_FALSE(Value(3.4028235e38).is_lossless_float());
    EXPECT_FALSE(Value(0.3).is_lossless_float());
}

TEST(Value, String) {
    // Construction with const string
    Value x("Hello", 5); // literal
    EXPECT_EQ(kStringType, x.get_type());
    EXPECT_TRUE(x.is_string());
    EXPECT_STREQ("Hello", x.get_string());
    EXPECT_EQ(5u, x.get_string_length());

    EXPECT_FALSE(x.is_number());
    EXPECT_FALSE(x.is_null());
    EXPECT_FALSE(x.is_bool());
    EXPECT_FALSE(x.is_false());
    EXPECT_FALSE(x.is_true());
    EXPECT_FALSE(x.is_object());
    EXPECT_FALSE(x.is_array());

    static const char cstr[] = "World"; // const array
    Value(cstr).swap(x);
    EXPECT_TRUE(x.is_string());
    EXPECT_EQ(x.get_string(), cstr);
    EXPECT_EQ(x.get_string_length(), sizeof(cstr)-1);

    static char mstr[] = "Howdy"; // non-const array
    // Value(mstr).swap(x); // should not compile
    Value(StringRef(mstr)).swap(x);
    EXPECT_TRUE(x.is_string());
    EXPECT_EQ(x.get_string(), mstr);
    EXPECT_EQ(x.get_string_length(), sizeof(mstr)-1);
    strncpy(mstr,"Hello", sizeof(mstr));
    EXPECT_STREQ(x.get_string(), "Hello");

    const char* pstr = cstr;
    //Value(pstr).swap(x); // should not compile
    Value(StringRef(pstr)).swap(x);
    EXPECT_TRUE(x.is_string());
    EXPECT_EQ(x.get_string(), cstr);
    EXPECT_EQ(x.get_string_length(), sizeof(cstr)-1);

    char* mpstr = mstr;
    Value(StringRef(mpstr,sizeof(mstr)-1)).swap(x);
    EXPECT_TRUE(x.is_string());
    EXPECT_EQ(x.get_string(), mstr);
    EXPECT_EQ(x.get_string_length(), 5u);
    EXPECT_STREQ(x.get_string(), "Hello");

    // Constructor with copy string
    MemoryPoolAllocator<> allocator;
    Value c(x.get_string(), x.get_string_length(), allocator);
    EXPECT_NE(x.get_string(), c.get_string());
    EXPECT_EQ(x.get_string_length(), c.get_string_length());
    EXPECT_STREQ(x.get_string(), c.get_string());
    //x.set_string("World");
    x.set_string("World", 5);
    EXPECT_STREQ("Hello", c.get_string());
    EXPECT_EQ(5u, c.get_string_length());

    // Constructor with type
    Value y(kStringType);
    EXPECT_TRUE(y.is_string());
    EXPECT_STREQ("", y.get_string());    // empty string should be "" instead of 0 (issue 226)
    EXPECT_EQ(0u, y.get_string_length());

    // SetConsttring()
    Value z;
    z.set_string("Hello");
    EXPECT_TRUE(x.is_string());
    z.set_string("Hello", 5);
    EXPECT_STREQ("Hello", z.get_string());
    EXPECT_STREQ("Hello", z.get_string());
    EXPECT_EQ(5u, z.get_string_length());

    z.set_string("Hello");
    EXPECT_TRUE(z.is_string());
    EXPECT_STREQ("Hello", z.get_string());

    //z.set_string(mstr); // should not compile
    //z.set_string(pstr); // should not compile
    z.set_string(StringRef(mstr));
    EXPECT_TRUE(z.is_string());
    EXPECT_STREQ(z.get_string(), mstr);

    z.set_string(cstr);
    EXPECT_TRUE(z.is_string());
    EXPECT_EQ(cstr, z.get_string());

    z = cstr;
    EXPECT_TRUE(z.is_string());
    EXPECT_EQ(cstr, z.get_string());

    // set_string()
    char s[] = "World";
    Value w;
    w.set_string(s, static_cast<SizeType>(strlen(s)), allocator);
    s[0] = '\0';
    EXPECT_STREQ("World", w.get_string());
    EXPECT_EQ(5u, w.get_string_length());

    // templated functions
    EXPECT_TRUE(z.is_type<const char*>());
    EXPECT_STREQ(cstr, z.get<const char*>());
    EXPECT_STREQ("Apple", z.set<const char*>("Apple").get<const char*>());

    {
        std::string str = "Hello World";
        str[5] = '\0';
        EXPECT_STREQ(str.data(),"Hello"); // embedded '\0'
        EXPECT_EQ(str.size(), 11u);

        // no copy
        Value vs0(StringRef(str));
        EXPECT_TRUE(vs0.is_string());
        EXPECT_EQ(vs0.get_string(), str.data());
        EXPECT_EQ(vs0.get_string_length(), str.size());
        TestEqual(vs0, str);

        // do copy
        Value vs1(str, allocator);
        EXPECT_TRUE(vs1.is_string());
        EXPECT_NE(vs1.get_string(), str.data());
        EXPECT_NE(vs1.get_string(), str); // not equal due to embedded '\0'
        EXPECT_EQ(vs1.get_string_length(), str.size());
        TestEqual(vs1, str);

        // set_string
        str = "World";
        vs0.SetNull().set_string(str, allocator);
        EXPECT_TRUE(vs0.is_string());
        EXPECT_STREQ(vs0.get_string(), str.c_str());
        EXPECT_EQ(vs0.get_string_length(), str.size());
        TestEqual(str, vs0);
        TestUnequal(str, vs1);

        // vs1 = str; // should not compile
        vs1 = StringRef(str);
        TestEqual(str, vs1);
        TestEqual(vs0, vs1);

        // Templated function.
        EXPECT_TRUE(vs0.is_type<std::string>());
        EXPECT_EQ(str, vs0.get<std::string>());
        vs0.set<std::string>(std::string("Apple"), allocator);
        EXPECT_EQ(std::string("Apple"), vs0.get<std::string>());
        vs0.set(std::string("Orange"), allocator);
        EXPECT_EQ(std::string("Orange"), vs0.get<std::string>());
    }

}

// Issue 226: Value of string type should not point to NULL
TEST(Value, SetStringNull) {

    MemoryPoolAllocator<> allocator;
    const char* nullPtr = 0;
    {
        // Construction with string type creates empty string
        Value v(kStringType);
        EXPECT_NE(v.get_string(), nullPtr); // non-null string returned
        EXPECT_EQ(v.get_string_length(), 0u);

        // Construction from/setting to null without length not allowed
        EXPECT_THROW(Value(StringRef(nullPtr)), AssertException);
        EXPECT_THROW(Value(StringRef(nullPtr), allocator), AssertException);
        EXPECT_THROW(v.set_string(nullPtr, allocator), AssertException);

        // Non-empty length with null string is not allowed
        EXPECT_THROW(v.set_string(nullPtr, 17u), AssertException);
        EXPECT_THROW(v.set_string(nullPtr, 42u, allocator), AssertException);

        // Setting to null string with empty length is allowed
        v.set_string(nullPtr, 0u);
        EXPECT_NE(v.get_string(), nullPtr); // non-null string returned
        EXPECT_EQ(v.get_string_length(), 0u);

        v.SetNull();
        v.set_string(nullPtr, 0u, allocator);
        EXPECT_NE(v.get_string(), nullPtr); // non-null string returned
        EXPECT_EQ(v.get_string_length(), 0u);
    }
    // Construction with null string and empty length is allowed
    {
        Value v(nullPtr,0u);
        EXPECT_NE(v.get_string(), nullPtr); // non-null string returned
        EXPECT_EQ(v.get_string_length(), 0u);
    }
    {
        Value v(nullPtr, 0u, allocator);
        EXPECT_NE(v.get_string(), nullPtr); // non-null string returned
        EXPECT_EQ(v.get_string_length(), 0u);
    }
}

template <typename T, typename Allocator>
static void TestArray(T& x, Allocator& allocator) {
    const T& y = x;

    // push_back()
    Value v;
    x.push_back(v, allocator);
    v.SetBool(true);
    x.push_back(v, allocator);
    v.SetBool(false);
    x.push_back(v, allocator);
    v.set_int32(123);
    x.push_back(v, allocator);
    //x.push_back((const char*)"foo", allocator); // should not compile
    x.push_back("foo", allocator);

    EXPECT_FALSE(x.empty());
    EXPECT_EQ(5u, x.size());
    EXPECT_FALSE(y.empty());
    EXPECT_EQ(5u, y.size());
    EXPECT_TRUE(x[SizeType(0)].is_null());
    EXPECT_TRUE(x[1].is_true());
    EXPECT_TRUE(x[2].is_false());
    EXPECT_TRUE(x[3].is_int32());
    EXPECT_EQ(123, x[3].get_int32());
    EXPECT_TRUE(y[SizeType(0)].is_null());
    EXPECT_TRUE(y[1].is_true());
    EXPECT_TRUE(y[2].is_false());
    EXPECT_TRUE(y[3].is_int32());
    EXPECT_EQ(123, y[3].get_int32());
    EXPECT_TRUE(y[4].is_string());
    EXPECT_STREQ("foo", y[4].get_string());

    // push_back(GenericValue&&, Allocator&);
    {
        Value y2(kArrayType);
        y2.push_back(Value(true), allocator);
        y2.push_back(std::move(Value(kArrayType).push_back(Value(1), allocator).push_back("foo", allocator)), allocator);
        EXPECT_EQ(2u, y2.size());
        EXPECT_TRUE(y2[0].is_true());
        EXPECT_TRUE(y2[1].is_array());
        EXPECT_EQ(2u, y2[1].size());
        EXPECT_TRUE(y2[1][0].is_int32());
        EXPECT_TRUE(y2[1][1].is_string());
    }

    // iterator
    typename T::ValueIterator itr = x.begin();
    EXPECT_TRUE(itr != x.end());
    EXPECT_TRUE(itr->is_null());
    ++itr;
    EXPECT_TRUE(itr != x.end());
    EXPECT_TRUE(itr->is_true());
    ++itr;
    EXPECT_TRUE(itr != x.end());
    EXPECT_TRUE(itr->is_false());
    ++itr;
    EXPECT_TRUE(itr != x.end());
    EXPECT_TRUE(itr->is_int32());
    EXPECT_EQ(123, itr->get_int32());
    ++itr;
    EXPECT_TRUE(itr != x.end());
    EXPECT_TRUE(itr->is_string());
    EXPECT_STREQ("foo", itr->get_string());

    // const iterator
    typename T::ConstValueIterator citr = y.begin();
    EXPECT_TRUE(citr != y.end());
    EXPECT_TRUE(citr->is_null());
    ++citr;
    EXPECT_TRUE(citr != y.end());
    EXPECT_TRUE(citr->is_true());
    ++citr;
    EXPECT_TRUE(citr != y.end());
    EXPECT_TRUE(citr->is_false());
    ++citr;
    EXPECT_TRUE(citr != y.end());
    EXPECT_TRUE(citr->is_int32());
    EXPECT_EQ(123, citr->get_int32());
    ++citr;
    EXPECT_TRUE(citr != y.end());
    EXPECT_TRUE(citr->is_string());
    EXPECT_STREQ("foo", citr->get_string());

    // pop_back()
    x.pop_back();
    EXPECT_EQ(4u, x.size());
    EXPECT_TRUE(y[SizeType(0)].is_null());
    EXPECT_TRUE(y[1].is_true());
    EXPECT_TRUE(y[2].is_false());
    EXPECT_TRUE(y[3].is_int32());

    // clear()
    x.clear();
    EXPECT_TRUE(x.empty());
    EXPECT_EQ(0u, x.size());
    EXPECT_TRUE(y.empty());
    EXPECT_EQ(0u, y.size());

    // erase(ValueIterator)

    // Use array of array to ensure removed elements' destructor is called.
    // [[0],[1],[2],...]
    for (int i = 0; i < 10; i++)
        x.push_back(Value(kArrayType).push_back(i, allocator).Move(), allocator);

    // Erase the first
    itr = x.erase(x.begin());
    EXPECT_EQ(x.begin(), itr);
    EXPECT_EQ(9u, x.size());
    for (int i = 0; i < 9; i++)
        EXPECT_EQ(i + 1, x[static_cast<SizeType>(i)][0].get_int32());

    // Ease the last
    itr = x.erase(x.end() - 1);
    EXPECT_EQ(x.end(), itr);
    EXPECT_EQ(8u, x.size());
    for (int i = 0; i < 8; i++)
        EXPECT_EQ(i + 1, x[static_cast<SizeType>(i)][0].get_int32());

    // Erase the middle
    itr = x.erase(x.begin() + 4);
    EXPECT_EQ(x.begin() + 4, itr);
    EXPECT_EQ(7u, x.size());
    for (int i = 0; i < 4; i++)
        EXPECT_EQ(i + 1, x[static_cast<SizeType>(i)][0].get_int32());
    for (int i = 4; i < 7; i++)
        EXPECT_EQ(i + 2, x[static_cast<SizeType>(i)][0].get_int32());

    // Erase(ValueIterator, ValueIterator)
    // Exhaustive test with all 0 <= first < n, first <= last <= n cases
    const unsigned n = 10;
    for (unsigned first = 0; first < n; first++) {
        for (unsigned last = first; last <= n; last++) {
            x.clear();
            for (unsigned i = 0; i < n; i++)
                x.push_back(Value(kArrayType).push_back(i, allocator).Move(), allocator);

            itr = x.erase(x.begin() + first, x.begin() + last);
            if (last == n)
                EXPECT_EQ(x.end(), itr);
            else
                EXPECT_EQ(x.begin() + first, itr);

            size_t removeCount = last - first;
            EXPECT_EQ(n - removeCount, x.size());
            for (unsigned i = 0; i < first; i++)
                EXPECT_EQ(i, x[i][0].get_uint32());
            for (unsigned i = first; i < n - removeCount; i++)
                EXPECT_EQ(i + removeCount, x[static_cast<SizeType>(i)][0].get_uint32());
        }
    }
}

TEST(Value, Array) {
    Value::AllocatorType allocator;
    Value x(kArrayType);
    const Value& y = x;

    EXPECT_EQ(kArrayType, x.get_type());
    EXPECT_TRUE(x.is_array());
    EXPECT_TRUE(x.empty());
    EXPECT_EQ(0u, x.size());
    EXPECT_TRUE(y.is_array());
    EXPECT_TRUE(y.empty());
    EXPECT_EQ(0u, y.size());

    EXPECT_FALSE(x.is_null());
    EXPECT_FALSE(x.is_bool());
    EXPECT_FALSE(x.is_false());
    EXPECT_FALSE(x.is_true());
    EXPECT_FALSE(x.is_string());
    EXPECT_FALSE(x.is_object());

    TestArray(x, allocator);

    // Working in gcc without C++11, but VS2013 cannot compile. To be diagnosed.
    // http://en.wikipedia.org/wiki/Erase-remove_idiom
    x.clear();
    for (int i = 0; i < 10; i++)
        if (i % 2 == 0)
            x.push_back(i, allocator);
        else
            x.push_back(Value(kNullType).Move(), allocator);

    const Value null(kNullType);
    x.erase(std::remove(x.begin(), x.end(), null), x.end());
    EXPECT_EQ(5u, x.size());
    for (int i = 0; i < 5; i++)
        EXPECT_EQ(i * 2, x[static_cast<SizeType>(i)]);

    // set_array()
    Value z;
    z.set_array();
    EXPECT_TRUE(z.is_array());
    EXPECT_TRUE(z.empty());

    // PR #1503: assign from inner Value
    {
        CrtAllocator a; // Free() is not a noop
        GenericValue<UTF8<>, CrtAllocator> nullValue;
        GenericValue<UTF8<>, CrtAllocator> arrayValue(kArrayType);
        arrayValue.push_back(nullValue, a);
        arrayValue = arrayValue[0]; // shouldn't crash (use after free)
        EXPECT_TRUE(arrayValue.is_null());
    }
}

TEST(Value, ArrayHelper) {
    Value::AllocatorType allocator;
    {
        Value x(kArrayType);
        Value::Array a = x.get_array();
        TestArray(a, allocator);
    }

    {
        Value x(kArrayType);
        Value::Array a = x.get_array();
        a.push_back(1, allocator);

        Value::Array a2(a); // copy constructor
        EXPECT_EQ(1u, a2.size());

        Value::Array a3 = a;
        EXPECT_EQ(1u, a3.size());

        Value::ConstArray y = static_cast<const Value&>(x).get_array();
        (void)y;
        // y.push_back(1, allocator); // should not compile

        // Templated functions
        x.clear();
        EXPECT_TRUE(x.is_type<Value::Array>());
        EXPECT_TRUE(x.is_type<Value::ConstArray>());
        a.push_back(1, allocator);
        EXPECT_EQ(1, x.get<Value::Array>()[0].get_int32());
        EXPECT_EQ(1, x.get<Value::ConstArray>()[0].get_int32());

        Value x2;
        x2.set<Value::Array>(a);
        EXPECT_TRUE(x.is_array());   // is_array() is invariant after moving.
        EXPECT_EQ(1, x2.get<Value::Array>()[0].get_int32());
    }

    {
        Value y(kArrayType);
        y.push_back(123, allocator);

        Value x(y.get_array());      // Construct value form array.
        EXPECT_TRUE(x.is_array());
        EXPECT_EQ(123, x[0].get_int32());
        EXPECT_TRUE(y.is_array());   // Invariant
        EXPECT_TRUE(y.empty());
    }

    {
        Value x(kArrayType);
        Value y(kArrayType);
        y.push_back(123, allocator);
        x.push_back(y.get_array(), allocator);    // Implicit constructor to convert Array to GenericValue

        EXPECT_EQ(1u, x.size());
        EXPECT_EQ(123, x[0][0].get_int32());
        EXPECT_TRUE(y.is_array());
        EXPECT_TRUE(y.empty());
    }
}

TEST(Value, ArrayHelperRangeFor) {
    Value::AllocatorType allocator;
    Value x(kArrayType);

    for (int i = 0; i < 10; i++)
        x.push_back(i, allocator);

    {
        int i = 0;
        for (auto& v : x.get_array()) {
            EXPECT_EQ(i, v.get_int32());
            i++;
        }
        EXPECT_EQ(i, 10);
    }
    {
        int i = 0;
        for (const auto& v : const_cast<const Value&>(x).get_array()) {
            EXPECT_EQ(i, v.get_int32());
            i++;
        }
        EXPECT_EQ(i, 10);
    }

    // Array a = x.get_array();
    // Array ca = const_cast<const Value&>(x).get_array();
}


template <typename T, typename Allocator>
static void TestObject(T& x, Allocator& allocator) {
    const T& y = x; // const version

    // add_member()
    x.add_member("A", "Apple", allocator);
    EXPECT_FALSE(x.object_empty());
    EXPECT_EQ(1u, x.member_count());

    Value value("Banana", 6);
    x.add_member("B", "Banana", allocator);
    EXPECT_EQ(2u, x.member_count());

    // add_member<T>(StringRefType, T, Allocator)
    {
        Value o(kObjectType);
        o.add_member("true", true, allocator);
        o.add_member("false", false, allocator);
        o.add_member("int", -1, allocator);
        o.add_member("uint", 1u, allocator);
        o.add_member("int64", int64_t(-4294967296), allocator);
        o.add_member("uint64", uint64_t(4294967296), allocator);
        o.add_member("double", 3.14, allocator);
        o.add_member("string", "Jelly", allocator);

        EXPECT_TRUE(o["true"].get_bool());
        EXPECT_FALSE(o["false"].get_bool());
        EXPECT_EQ(-1, o["int"].get_int32());
        EXPECT_EQ(1u, o["uint"].get_uint32());
        EXPECT_EQ(int64_t(-4294967296), o["int64"].get_int64());
        EXPECT_EQ(uint64_t(4294967296), o["uint64"].get_uint64());
        EXPECT_STREQ("Jelly",o["string"].get_string());
        EXPECT_EQ(8u, o.member_count());
    }

    // add_member<T>(Value&, T, Allocator)
    {
        Value o(kObjectType);

        Value n("s");
        o.add_member(n, "string", allocator);
        EXPECT_EQ(1u, o.member_count());

        Value count("#");
        o.add_member(count, o.member_count(), allocator);
        EXPECT_EQ(2u, o.member_count());
    }

    {
        // add_member(StringRefType, const std::string&, Allocator)
        Value o(kObjectType);
        o.add_member("b", std::string("Banana"), allocator);
        EXPECT_STREQ("Banana", o["b"].get_string());

        // remove_member(const std::string&)
        o.remove_member(std::string("b"));
        EXPECT_TRUE(o.object_empty());
    }

    // add_member(GenericValue&&, ...) variants
    {
        Value o(kObjectType);
        o.add_member(Value("true"), Value(true), allocator);
        o.add_member(Value("false"), Value(false).Move(), allocator);    // value is lvalue ref
        o.add_member(Value("int").Move(), Value(-1), allocator);         // name is lvalue ref
        o.add_member("uint", std::move(Value().set_uint32(1u)), allocator); // name is literal, value is rvalue
        EXPECT_TRUE(o["true"].get_bool());
        EXPECT_FALSE(o["false"].get_bool());
        EXPECT_EQ(-1, o["int"].get_int32());
        EXPECT_EQ(1u, o["uint"].get_uint32());
        EXPECT_EQ(4u, o.member_count());
    }

    // Tests a member with null character
    Value name;
    const Value C0D("C\0D", 3);
    name.set_string(C0D.get_string(), 3);
    value.set_string("CherryD", 7);
    x.add_member(name, value, allocator);

    // has_member()
    EXPECT_TRUE(x.has_member("A"));
    EXPECT_TRUE(x.has_member("B"));
    EXPECT_TRUE(y.has_member("A"));
    EXPECT_TRUE(y.has_member("B"));


    EXPECT_TRUE(x.has_member(std::string("A")));


    name.set_string("C\0D");
    EXPECT_TRUE(x.has_member(name));
    EXPECT_TRUE(y.has_member(name));

    GenericValue<UTF8<>, CrtAllocator> othername("A");
    EXPECT_TRUE(x.has_member(othername));
    EXPECT_TRUE(y.has_member(othername));
    othername.set_string("C\0D");
    EXPECT_TRUE(x.has_member(othername));
    EXPECT_TRUE(y.has_member(othername));

    // operator[]
    EXPECT_STREQ("Apple", x["A"].get_string());
    EXPECT_STREQ("Banana", x["B"].get_string());
    EXPECT_STREQ("CherryD", x[C0D].get_string());
    EXPECT_STREQ("CherryD", x[othername].get_string());
    EXPECT_THROW(x["nonexist"], AssertException);

    // const operator[]
    EXPECT_STREQ("Apple", y["A"].get_string());
    EXPECT_STREQ("Banana", y["B"].get_string());
    EXPECT_STREQ("CherryD", y[C0D].get_string());


    EXPECT_STREQ("Apple", x["A"].get_string());
    EXPECT_STREQ("Apple", y[std::string("A")].get_string());


    // member iterator
    Value::MemberIterator itr = x.member_begin();
    EXPECT_TRUE(itr != x.member_end());
    EXPECT_STREQ("A", itr->name.get_string());
    EXPECT_STREQ("Apple", itr->value.get_string());
    ++itr;
    EXPECT_TRUE(itr != x.member_end());
    EXPECT_STREQ("B", itr->name.get_string());
    EXPECT_STREQ("Banana", itr->value.get_string());
    ++itr;
    EXPECT_TRUE(itr != x.member_end());
    EXPECT_TRUE(memcmp(itr->name.get_string(), "C\0D", 4) == 0);
    EXPECT_STREQ("CherryD", itr->value.get_string());
    ++itr;
    EXPECT_FALSE(itr != x.member_end());

    // const member iterator
    Value::ConstMemberIterator citr = y.member_begin();
    EXPECT_TRUE(citr != y.member_end());
    EXPECT_STREQ("A", citr->name.get_string());
    EXPECT_STREQ("Apple", citr->value.get_string());
    ++citr;
    EXPECT_TRUE(citr != y.member_end());
    EXPECT_STREQ("B", citr->name.get_string());
    EXPECT_STREQ("Banana", citr->value.get_string());
    ++citr;
    EXPECT_TRUE(citr != y.member_end());
    EXPECT_TRUE(memcmp(citr->name.get_string(), "C\0D", 4) == 0);
    EXPECT_STREQ("CherryD", citr->value.get_string());
    ++citr;
    EXPECT_FALSE(citr != y.member_end());

    // member iterator conversions/relations
    itr  = x.member_begin();
    citr = x.member_begin(); // const conversion
    TestEqual(itr, citr);
    EXPECT_TRUE(itr < x.member_end());
    EXPECT_FALSE(itr > y.member_end());
    EXPECT_TRUE(citr < x.member_end());
    EXPECT_FALSE(citr > y.member_end());
    ++citr;
    TestUnequal(itr, citr);
    EXPECT_FALSE(itr < itr);
    EXPECT_TRUE(itr < citr);
    EXPECT_FALSE(itr > itr);
    EXPECT_TRUE(citr > itr);
    EXPECT_EQ(1, citr - x.member_begin());
    EXPECT_EQ(0, itr - y.member_begin());
    itr += citr - x.member_begin();
    EXPECT_EQ(1, itr - y.member_begin());
    TestEqual(citr, itr);
    EXPECT_TRUE(itr <= citr);
    EXPECT_TRUE(citr <= itr);
    itr++;
    EXPECT_TRUE(itr >= citr);
    EXPECT_FALSE(citr >= itr);

    // remove_member()
    EXPECT_TRUE(x.remove_member("A"));
    EXPECT_FALSE(x.has_member("A"));

    EXPECT_TRUE(x.remove_member("B"));
    EXPECT_FALSE(x.has_member("B"));

    EXPECT_FALSE(x.remove_member("nonexist"));

    EXPECT_TRUE(x.remove_member(othername));
    EXPECT_FALSE(x.has_member(name));

    EXPECT_TRUE(x.member_begin() == x.member_end());

    // erase_member(ConstMemberIterator)

    // Use array members to ensure removed elements' destructor is called.
    // { "a": [0], "b": [1],[2],...]
    const char keys[][2] = { "a", "b", "c", "d", "e", "f", "g", "h", "i", "j" };
    for (int i = 0; i < 10; i++)
        x.add_member(keys[i], Value(kArrayType).push_back(i, allocator), allocator);

    // member_count, iterator difference
    EXPECT_EQ(x.member_count(), SizeType(x.member_end() - x.member_begin()));

    // erase the first
    itr = x.erase_member(x.member_begin());
    EXPECT_FALSE(x.has_member(keys[0]));
    EXPECT_EQ(x.member_begin(), itr);
    EXPECT_EQ(9u, x.member_count());
    for (; itr != x.member_end(); ++itr) {
        size_t i = static_cast<size_t>((itr - x.member_begin())) + 1;
        EXPECT_STREQ(itr->name.get_string(), keys[i]);
        EXPECT_EQ(static_cast<int>(i), itr->value[0].get_int32());
    }

    // Erase the last
    itr = x.erase_member(x.member_end() - 1);
    EXPECT_FALSE(x.has_member(keys[9]));
    EXPECT_EQ(x.member_end(), itr);
    EXPECT_EQ(8u, x.member_count());
    for (; itr != x.member_end(); ++itr) {
        size_t i = static_cast<size_t>(itr - x.member_begin()) + 1;
        EXPECT_STREQ(itr->name.get_string(), keys[i]);
        EXPECT_EQ(static_cast<int>(i), itr->value[0].get_int32());
    }

    // Erase the middle
    itr = x.erase_member(x.member_begin() + 4);
    EXPECT_FALSE(x.has_member(keys[5]));
    EXPECT_EQ(x.member_begin() + 4, itr);
    EXPECT_EQ(7u, x.member_count());
    for (; itr != x.member_end(); ++itr) {
        size_t i = static_cast<size_t>(itr - x.member_begin());
        i += (i < 4) ? 1 : 2;
        EXPECT_STREQ(itr->name.get_string(), keys[i]);
        EXPECT_EQ(static_cast<int>(i), itr->value[0].get_int32());
    }

    // erase_member(ConstMemberIterator, ConstMemberIterator)
    // Exhaustive test with all 0 <= first < n, first <= last <= n cases
    const unsigned n = 10;
    for (unsigned first = 0; first < n; first++) {
        for (unsigned last = first; last <= n; last++) {
            x.RemoveAllMembers();
            for (unsigned i = 0; i < n; i++)
                x.add_member(keys[i], Value(kArrayType).push_back(i, allocator), allocator);

            itr = x.erase_member(x.member_begin() + static_cast<int>(first), x.member_begin() + static_cast<int>(last));
            if (last == n)
                EXPECT_EQ(x.member_end(), itr);
            else
                EXPECT_EQ(x.member_begin() + static_cast<int>(first), itr);

            size_t removeCount = last - first;
            EXPECT_EQ(n - removeCount, x.member_count());
            for (unsigned i = 0; i < first; i++)
                EXPECT_EQ(i, x[keys[i]][0].get_uint32());
            for (unsigned i = first; i < n - removeCount; i++)
                EXPECT_EQ(i + removeCount, x[keys[i+removeCount]][0].get_uint32());
        }
    }

    // RemoveAllMembers()
    x.RemoveAllMembers();
    EXPECT_TRUE(x.object_empty());
    EXPECT_EQ(0u, x.member_count());
}

TEST(Value, Object) {
    Value::AllocatorType allocator;
    Value x(kObjectType);
    const Value& y = x; // const version

    EXPECT_EQ(kObjectType, x.get_type());
    EXPECT_TRUE(x.is_object());
    EXPECT_TRUE(x.object_empty());
    EXPECT_EQ(0u, x.member_count());
    EXPECT_EQ(kObjectType, y.get_type());
    EXPECT_TRUE(y.is_object());
    EXPECT_TRUE(y.object_empty());
    EXPECT_EQ(0u, y.member_count());

    TestObject(x, allocator);

    // SetObject()
    Value z;
    z.SetObject();
    EXPECT_TRUE(z.is_object());
}

TEST(Value, ObjectHelper) {
    Value::AllocatorType allocator;
    {
        Value x(kObjectType);
        Value::Object o = x.get_object();
        TestObject(o, allocator);
    }

    {
        Value x(kObjectType);
        Value::Object o = x.get_object();
        o.add_member("1", 1, allocator);

        Value::Object o2(o); // copy constructor
        EXPECT_EQ(1u, o2.member_count());

        Value::Object o3 = o;
        EXPECT_EQ(1u, o3.member_count());

        Value::ConstObject y = static_cast<const Value&>(x).get_object();
        (void)y;
        // y.add_member("1", 1, allocator); // should not compile

        // Templated functions
        x.RemoveAllMembers();
        EXPECT_TRUE(x.is_type<Value::Object>());
        EXPECT_TRUE(x.is_type<Value::ConstObject>());
        o.add_member("1", 1, allocator);
        EXPECT_EQ(1, x.get<Value::Object>()["1"].get_int32());
        EXPECT_EQ(1, x.get<Value::ConstObject>()["1"].get_int32());

        Value x2;
        x2.set<Value::Object>(o);
        EXPECT_TRUE(x.is_object());   // is_object() is invariant after moving
        EXPECT_EQ(1, x2.get<Value::Object>()["1"].get_int32());
    }

    {
        Value x(kObjectType);
        x.add_member("a", "apple", allocator);
        Value y(x.get_object());
        EXPECT_STREQ("apple", y["a"].get_string());
        EXPECT_TRUE(x.is_object());  // Invariant
    }

    {
        Value x(kObjectType);
        x.add_member("a", "apple", allocator);
        Value y(kObjectType);
        y.add_member("fruits", x.get_object(), allocator);
        EXPECT_STREQ("apple", y["fruits"]["a"].get_string());
        EXPECT_TRUE(x.is_object());  // Invariant
    }
}

TEST(Value, ObjectHelperRangeFor) {
    Value::AllocatorType allocator;
    Value x(kObjectType);

    for (int i = 0; i < 10; i++) {
        char name[10];
        Value n(name, static_cast<SizeType>(sprintf(name, "%d", i)), allocator);
        x.add_member(n, i, allocator);
    }

    {
        int i = 0;
        for (auto& m : x.get_object()) {
            char name[11];
            sprintf(name, "%d", i);
            EXPECT_STREQ(name, m.name.get_string());
            EXPECT_EQ(i, m.value.get_int32());
            i++;
        }
        EXPECT_EQ(i, 10);
    }
    {
        int i = 0;
        for (const auto& m : const_cast<const Value&>(x).get_object()) {
            char name[11];
            sprintf(name, "%d", i);
            EXPECT_STREQ(name, m.name.get_string());
            EXPECT_EQ(i, m.value.get_int32());
            i++;
        }
        EXPECT_EQ(i, 10);
    }

    // Object a = x.get_object();
    // Object ca = const_cast<const Value&>(x).get_object();
}

TEST(Value, EraseMember_String) {
    Value::AllocatorType allocator;
    Value x(kObjectType);
    x.add_member("A", "Apple", allocator);
    x.add_member("B", "Banana", allocator);

    EXPECT_TRUE(x.erase_member("B"));
    EXPECT_FALSE(x.has_member("B"));

    EXPECT_FALSE(x.erase_member("nonexist"));

    GenericValue<UTF8<>, CrtAllocator> othername("A");
    EXPECT_TRUE(x.erase_member(othername));
    EXPECT_FALSE(x.has_member("A"));

    EXPECT_TRUE(x.member_begin() == x.member_end());
}

TEST(Value, BigNestedArray) {
    MemoryPoolAllocator<> allocator;
    Value x(kArrayType);
    static const SizeType  n = 200;

    for (SizeType i = 0; i < n; i++) {
        Value y(kArrayType);
        for (SizeType  j = 0; j < n; j++) {
            Value number(static_cast<int>(i * n + j));
            y.push_back(number, allocator);
        }
        x.push_back(y, allocator);
    }

    for (SizeType i = 0; i < n; i++)
        for (SizeType j = 0; j < n; j++) {
            EXPECT_TRUE(x[i][j].is_int32());
            EXPECT_EQ(static_cast<int>(i * n + j), x[i][j].get_int32());
        }
}

TEST(Value, BigNestedObject) {
    MemoryPoolAllocator<> allocator;
    Value x(kObjectType);
    static const SizeType n = 200;
    const char* format = std::numeric_limits<SizeType>::is_signed ? "%d" : "%u";

    for (SizeType i = 0; i < n; i++) {
        char name1[10];
        sprintf(name1, format, i);

        // Value name(name1); // should not compile
        Value name(name1, static_cast<SizeType>(strlen(name1)), allocator);
        Value object(kObjectType);

        for (SizeType j = 0; j < n; j++) {
            char name2[10];
            sprintf(name2, format, j);

            Value name3(name2, static_cast<SizeType>(strlen(name2)), allocator);
            Value number(static_cast<int>(i * n + j));
            object.add_member(name3, number, allocator);
        }

        // x.add_member(name1, object, allocator); // should not compile
        x.add_member(name, object, allocator);
    }

    for (SizeType i = 0; i < n; i++) {
        char name1[10];
        sprintf(name1, format, i);

        for (SizeType j = 0; j < n; j++) {
            char name2[10];
            sprintf(name2, format, j);
            x[name1];
            EXPECT_EQ(static_cast<int>(i * n + j), x[name1][name2].get_int32());
        }
    }
}

// Issue 18: Error removing last element of object
// http://code.google.com/p/rapidjson/issues/detail?id=18
TEST(Value, RemoveLastElement) {
    merak::json::Document doc;
    merak::json::Document::AllocatorType& allocator = doc.get_allocator();
    merak::json::Value objVal(merak::json::kObjectType);
    objVal.add_member("var1", 123, allocator);
    objVal.add_member("var2", "444", allocator);
    objVal.add_member("var3", 555, allocator);
    EXPECT_TRUE(objVal.has_member("var3"));
    objVal.remove_member("var3");    // Assertion here in r61
    EXPECT_FALSE(objVal.has_member("var3"));
}

// Issue 38:    Segmentation fault with CrtAllocator
TEST(Document, CrtAllocator) {
    typedef GenericValue<UTF8<>, CrtAllocator> V;

    V::AllocatorType allocator;
    V o(kObjectType);
    o.add_member("x", 1, allocator); // Should not call destructor on uninitialized name/value of newly allocated members.

    V a(kArrayType);
    a.push_back(1, allocator);   // Should not call destructor on uninitialized Value of newly allocated elements.
}

static void TestShortStringOptimization(const char* str) {
    const merak::json::SizeType len = static_cast<merak::json::SizeType>(strlen(str));

    merak::json::Document doc;
    merak::json::Value val;
    val.set_string(str, len, doc.get_allocator());

    EXPECT_EQ(val.get_string_length(), len);
    EXPECT_STREQ(val.get_string(), str);
}

TEST(Value, AllocateShortString) {
    TestShortStringOptimization("");                 // edge case: empty string
    TestShortStringOptimization("12345678");         // regular case for short strings: 8 chars
    TestShortStringOptimization("12345678901");      // edge case: 11 chars in 32-bit mode (=> short string)
    TestShortStringOptimization("123456789012");     // edge case: 12 chars in 32-bit mode (=> regular string)
    TestShortStringOptimization("123456789012345");  // edge case: 15 chars in 64-bit mode (=> short string)
    TestShortStringOptimization("1234567890123456"); // edge case: 16 chars in 64-bit mode (=> regular string)
}

template <int e>
struct TerminateHandler {
    bool emplace_null() { return e != 0; }
    bool emplace_bool(bool) { return e != 1; }
    bool emplace_int32(int) { return e != 2; }
    bool emplace_uint32(unsigned) { return e != 3; }
    bool emplace_int64(int64_t) { return e != 4; }
    bool emplace_uint64(uint64_t) { return e != 5; }
    bool emplace_double(double) { return e != 6; }
    bool raw_number(const char*, SizeType, bool) { return e != 7; }
    bool emplace_string(const char*, SizeType, bool) { return e != 8; }
    bool start_object() { return e != 9; }
    bool emplace_key(const char*, SizeType, bool)  { return e != 10; }
    bool end_object(SizeType) { return e != 11; }
    bool start_array() { return e != 12; }
    bool end_array(SizeType) { return e != 13; }
};

#define TEST_TERMINATION(e, json)\
{\
    Document d; \
    EXPECT_FALSE(d.parse(json).has_parse_error()); \
    Reader reader; \
    TerminateHandler<e> h;\
    EXPECT_FALSE(d.accept(h));\
}

TEST(Value, AcceptTerminationByHandler) {
    TEST_TERMINATION(0, "[null]");
    TEST_TERMINATION(1, "[true]");
    TEST_TERMINATION(1, "[false]");
    TEST_TERMINATION(2, "[-1]");
    TEST_TERMINATION(3, "[2147483648]");
    TEST_TERMINATION(4, "[-1234567890123456789]");
    TEST_TERMINATION(5, "[9223372036854775808]");
    TEST_TERMINATION(6, "[0.5]");
    // raw_number() is never called
    TEST_TERMINATION(8, "[\"a\"]");
    TEST_TERMINATION(9, "[{}]");
    TEST_TERMINATION(10, "[{\"a\":1}]");
    TEST_TERMINATION(11, "[{}]");
    TEST_TERMINATION(12, "{\"a\":[]}");
    TEST_TERMINATION(13, "{\"a\":[]}");
}

struct ValueIntComparer {
    bool operator()(const Value& lhs, const Value& rhs) const {
        return lhs.get_int32() < rhs.get_int32();
    }
};

TEST(Value, Sorting) {
    Value::AllocatorType allocator;
    Value a(kArrayType);
    a.push_back(5, allocator);
    a.push_back(1, allocator);
    a.push_back(3, allocator);
    std::sort(a.begin(), a.end(), ValueIntComparer());
    EXPECT_EQ(1, a[0].get_int32());
    EXPECT_EQ(3, a[1].get_int32());
    EXPECT_EQ(5, a[2].get_int32());
}

// http://stackoverflow.com/questions/35222230/

static void MergeDuplicateKey(Value& v, Value::AllocatorType& a) {
    if (v.is_object()) {
        // Convert all key:value into key:[value]
        for (Value::MemberIterator itr = v.member_begin(); itr != v.member_end(); ++itr)
            itr->value = Value(kArrayType).Move().push_back(itr->value, a);

        // Merge arrays if key is duplicated
        for (Value::MemberIterator itr = v.member_begin(); itr != v.member_end();) {
            Value::MemberIterator itr2 = v.find_member(itr->name);
            if (itr != itr2) {
                itr2->value.push_back(itr->value[0], a);
                itr = v.erase_member(itr);
            }
            else
                ++itr;
        }

        // Convert key:[values] back to key:value if there is only one value
        for (Value::MemberIterator itr = v.member_begin(); itr != v.member_end(); ++itr) {
            if (itr->value.size() == 1)
                itr->value = itr->value[0];
            MergeDuplicateKey(itr->value, a); // Recursion on the value
        }
    }
    else if (v.is_array())
        for (Value::ValueIterator itr = v.begin(); itr != v.end(); ++itr)
            MergeDuplicateKey(*itr, a);
}

TEST(Value, MergeDuplicateKey) {
    Document d;
    d.parse(
        "{"
        "    \"key1\": {"
        "        \"a\": \"asdf\","
        "        \"b\": \"foo\","
        "        \"b\": \"bar\","
        "        \"c\": \"fdas\""
        "    }"
        "}");

    Document d2;
    d2.parse(
        "{"
        "    \"key1\": {"
        "        \"a\": \"asdf\","
        "        \"b\": ["
        "            \"foo\","
        "            \"bar\""
        "        ],"
        "        \"c\": \"fdas\""
        "    }"
        "}");

    EXPECT_NE(d2, d);
    MergeDuplicateKey(d, d.get_allocator());
    EXPECT_EQ(d2, d);
}

TEST(Value, SSOMemoryOverlapTest) {
    Document d;
    d.parse("{\"project\":\"rapidjson\",\"stars\":\"ssovalue\"}");
    Value &s = d["stars"];
    s.set_string(GenericStringRef<char>(&(s.get_string()[1]), 5), d.get_allocator());
    EXPECT_TRUE(strcmp(s.get_string(),"soval") == 0);
}

#ifdef __clang__
MERAK_JSON_DIAG_POP
#endif
