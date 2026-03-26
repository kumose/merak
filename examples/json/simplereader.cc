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
