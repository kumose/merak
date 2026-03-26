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
#include <merak/json/document.h>
#include <merak/json/filereadstream.h>
#include <merak/json/pointer.h>
#include <merak/json.h>
#include <iostream>

using namespace merak::json;

void traverse(const Value& v, const Pointer& p) {
    StringBuffer sb;
    p.Stringify(sb);
    std::cout << sb.get_string() << std::endl;

    switch (v.get_type()) {
    case kArrayType:
        for (SizeType i = 0; i != v.size(); ++i)
            traverse(v[i], p.Append(i));
        break;
    case kObjectType:
        for (Value::ConstMemberIterator m = v.member_begin(); m != v.member_end(); ++m)
            traverse(m->value, p.Append(m->name.get_string(), m->name.get_string_length()));
        break;
    default:
        break;
    }
}

int main(int, char*[]) {
    char readBuffer[65536];
    FileReadStream is(stdin, readBuffer, sizeof(readBuffer));

    Document d;
    d.parse_stream(is);

    Pointer root;
    traverse(d, root);

    return 0;
}
