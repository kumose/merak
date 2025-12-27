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
#include <merak/json.h>
#include <iostream>
#include <sstream>

using namespace merak::json;
using namespace std;

// If you can require C++11, you could use std::to_string here
template <typename T> std::string stringify(T x) {
    std::stringstream ss;
    ss << x;
    return ss.str();
}

struct MyHandler {
    const char* type;
    std::string data;
    
    MyHandler() : type(), data() {}

    bool emplace_null() { type = "Null"; data.clear(); return true; }
    bool emplace_bool(bool b) { type = "Bool:"; data = b? "true": "false"; return true; }
    bool emplace_int32(int i) { type = "Int:"; data = stringify(i); return true; }
    bool emplace_uint32(unsigned u) { type = "Uint:"; data = stringify(u); return true; }
    bool emplace_int64(int64_t i) { type = "Int64:"; data = stringify(i); return true; }
    bool emplace_uint64(uint64_t u) { type = "Uint64:"; data = stringify(u); return true; }
    bool emplace_double(double d) { type = "Double:"; data = stringify(d); return true; }
    bool raw_number(const char* str, SizeType length, bool) { type = "Number:"; data = std::string(str, length); return true; }
    bool emplace_string(const char* str, SizeType length, bool) { type = "String:"; data = std::string(str, length); return true; }
    bool start_object() { type = "start_object"; data.clear(); return true; }
    bool emplace_key(const char* str, SizeType length, bool) { type = "Key:"; data = std::string(str, length); return true; }
    bool end_object(SizeType memberCount) { type = "end_object:"; data = stringify(memberCount); return true; }
    bool start_array() { type = "start_array"; data.clear(); return true; }
    bool end_array(SizeType elementCount) { type = "end_array:"; data = stringify(elementCount); return true; }
private:
    MyHandler(const MyHandler& noCopyConstruction);
    MyHandler& operator=(const MyHandler& noAssignment);
};

int main() {
    const char json[] = " { \"hello\" : \"world\", \"t\" : true , \"f\" : false, \"n\": null, \"i\":123, \"pi\": 3.1416, \"a\":[1, 2, 3, 4] } ";

    MyHandler handler;
    Reader reader;
    StringStream ss(json);
    reader.IterativeParseInit();
    while (!reader.IterativeParseComplete()) {
        reader.IterativeParseNext<kParseDefaultFlags>(ss, handler);
        cout << handler.type << handler.data << endl;
    }

    return 0;
}
