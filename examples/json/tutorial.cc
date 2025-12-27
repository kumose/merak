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
// Hello World example
// This example shows basic usage of DOM-style API.

#include <merak/json.h>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <turbo/strings/string_builder.h>

using namespace merak::json;
using namespace std;

int main(int, char*[]) {
    ////////////////////////////////////////////////////////////////////////////
    // 1. parse a JSON text string to a document.

    const char json[] = " { \"hello\" : \"world\", \"t\" : true , \"f\" : false, \"n\": null, \"i\":123, \"pi\": 3.1416, \"a\":[1, 2, 3, 4] } ";
    printf("Original JSON:\n %s\n", json);

    Document document;  // Default template parameter uses UTF8 and MemoryPoolAllocator.

    // "normal" parsing, decode strings to new buffers. Can use other input stream via parse_stream().
    if (document.parse(json).has_parse_error())
        return 1;

    printf("\nParsing to document succeeded.\n");

    ////////////////////////////////////////////////////////////////////////////
    // 2. Access values in document. 

    printf("\nAccess values in document:\n");
    assert(document.is_object());    // Document is a JSON value represents the root of DOM. Root can be either an object or array.

    assert(document.has_member("hello"));
    assert(document["hello"].is_string());
    printf("hello = %s\n", document["hello"].get_string());

    // Since version 0.2, you can use single lookup to check the existing of member and its value:
    Value::MemberIterator hello = document.find_member("hello");
    assert(hello != document.member_end());
    assert(hello->value.is_string());
    assert(strcmp("world", hello->value.get_string()) == 0);
    (void)hello;

    assert(document["t"].is_bool());     // JSON true/false are bool. Can also uses more specific function is_true().
    printf("t = %s\n", document["t"].get_bool() ? "true" : "false");

    assert(document["f"].is_bool());
    printf("f = %s\n", document["f"].get_bool() ? "true" : "false");

    printf("n = %s\n", document["n"].is_null() ? "null" : "?");

    assert(document["i"].is_number());   // Number is a JSON type, but C++ needs more specific type.
    assert(document["i"].is_int32());      // In this case, is_uint32()/is_int64()/is_uint64() also return true.
    printf("i = %d\n", document["i"].get_int32()); // Alternative (int)document["i"]

    assert(document["pi"].is_number());
    assert(document["pi"].is_double());
    printf("pi = %g\n", document["pi"].get_double());

    {
        const Value& a = document["a"]; // Using a reference for consecutive access is handy and faster.
        assert(a.is_array());
        for (SizeType i = 0; i < a.size(); i++) // rapidjson uses SizeType instead of size_t.
            printf("a[%d] = %d\n", i, a[i].get_int32());
        
        int y = a[0].get_int32();
        (void)y;

        // Iterating array with iterators
        printf("a = ");
        for (Value::ConstValueIterator itr = a.begin(); itr != a.end(); ++itr)
            printf("%d ", itr->get_int32());
        printf("\n");
    }

    // Iterating object members
    static const char* kTypeNames[] = { "Null", "False", "True", "Object", "Array", "String", "Number" };
    for (Value::ConstMemberIterator itr = document.member_begin(); itr != document.member_end(); ++itr)
        printf("Type of member %s is %s\n", itr->name.get_string(), kTypeNames[itr->value.get_type()]);

    ////////////////////////////////////////////////////////////////////////////
    // 3. Modify values in document.

    // Change i to a bigger number
    {
        uint64_t f20 = 1;   // compute factorial of 20
        for (uint64_t j = 1; j <= 20; j++)
            f20 *= j;
        document["i"] = f20;    // Alternate form: document["i"].set_uint64(f20)
        assert(!document["i"].is_int32()); // No longer can be cast as int or uint.
    }

    // Adding values to array.
    {
        Value& a = document["a"];   // This time we uses non-const reference.
        Document::AllocatorType& allocator = document.get_allocator();
        for (int i = 5; i <= 10; i++)
            a.push_back(i, allocator);   // May look a bit strange, allocator is needed for potentially realloc. We normally uses the document's.

        // Fluent API
        a.push_back("Lua", allocator).push_back("Mio", allocator);
    }

    // Making string values.

    // This version of set_string() just store the pointer to the string.
    // So it is for literal and string that exists within value's life-cycle.
    {
        document["hello"] = "rapidjson";    // This will invoke strlen()
        // Faster version:
        // document["hello"].set_string("rapidjson", 9);
    }

    // This version of set_string() needs an allocator, which means it will allocate a new buffer and copy the the string into the buffer.
    Value author;
    {
        char buffer2[10];
        int len = sprintf(buffer2, "%s %s", "Milo", "Yip");  // synthetic example of dynamically created string.

        author.set_string(buffer2, static_cast<SizeType>(len), document.get_allocator());
        // Shorter but slower version:
        // document["hello"].set_string(buffer, document.get_allocator());

        // Constructor version: 
        // Value author(buffer, len, document.get_allocator());
        // Value author(buffer, document.get_allocator());
        memset(buffer2, 0, sizeof(buffer2)); // For demonstration purpose.
    }
    // Variable 'buffer' is unusable now but 'author' has already made a copy.
    document.add_member("author", author, document.get_allocator());

    assert(author.is_null());        // Move semantic for assignment. After this variable is assigned as a member, the variable becomes null.

    ////////////////////////////////////////////////////////////////////////////
    // 4. Stringify JSON

    printf("\nModified JSON with reformatting:\n");
    StringBuffer sb;
    PrettyWriter<StringBuffer> writer(sb);
    document.accept(writer);    // accept() traverses the DOM and generates Handler events.
    puts(sb.get_string());

    std::cout<<std::string('#', 30)<<std::endl;
    std::cout<<document.to_string()<<std::endl;
    std::cout<<std::string('#', 30)<<std::endl;
    std::stringstream ss;
    document.describe(ss);
    std::cout<<ss.str()<<std::endl;
    std::cout<<std::string('#', 30)<<std::endl;
    std::cout<<turbo::StringBuilder::create(document)<<std::endl;
    return 0;
}
