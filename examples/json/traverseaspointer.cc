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
