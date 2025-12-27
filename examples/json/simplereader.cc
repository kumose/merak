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

using namespace merak::json;
using namespace std;

struct MyHandler {
    bool emplace_null() { cout << "emplace_null()" << endl; return true; }
    bool emplace_bool(bool b) { cout << "emplace_bool(" << boolalpha << b << ")" << endl; return true; }
    bool emplace_int32(int i) { cout << "emplace_int32(" << i << ")" << endl; return true; }
    bool emplace_uint32(unsigned u) { cout << "emplace_uint32(" << u << ")" << endl; return true; }
    bool emplace_int64(int64_t i) { cout << "emplace_int64(" << i << ")" << endl; return true; }
    bool emplace_uint64(uint64_t u) { cout << "emplace_uint64(" << u << ")" << endl; return true; }
    bool emplace_double(double d) { cout << "emplace_double(" << d << ")" << endl; return true; }
    bool raw_number(const char* str, SizeType length, bool copy) {
        cout << "Number(" << str << ", " << length << ", " << boolalpha << copy << ")" << endl;
        return true;
    }
    bool emplace_string(const char* str, SizeType length, bool copy) {
        cout << "emplace_string(" << str << ", " << length << ", " << boolalpha << copy << ")" << endl;
        return true;
    }
    bool start_object() { cout << "start_object()" << endl; return true; }
    bool emplace_key(const char* str, SizeType length, bool copy) {
        cout << "emplace_key(" << str << ", " << length << ", " << boolalpha << copy << ")" << endl;
        return true;
    }
    bool end_object(SizeType memberCount) { cout << "end_object(" << memberCount << ")" << endl; return true; }
    bool start_array() { cout << "start_array()" << endl; return true; }
    bool end_array(SizeType elementCount) { cout << "end_array(" << elementCount << ")" << endl; return true; }
};

int main() {
    const char json[] = " { \"hello\" : \"world\", \"t\" : true , \"f\" : false, \"n\": null, \"i\":123, \"pi\": 3.1416, \"a\":[1, 2, 3, 4] } ";

    MyHandler handler;
    Reader reader;
    StringStream ss(json);
    reader.parse(ss, handler);

    return 0;
}
