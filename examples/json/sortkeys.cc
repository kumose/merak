
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

#include <algorithm>
#include <iostream>

using namespace merak::json;
using namespace std;

static void printIt(const Value &doc) {
    char writeBuffer[65536];
    FileWriteStream os(stdout, writeBuffer, sizeof(writeBuffer));
    PrettyWriter<FileWriteStream> writer(os);
    doc.accept(writer);
    cout << endl;
}

struct NameComparator {
    bool operator()(const Value::Member &lhs, const Value::Member &rhs) const {
        return (strcmp(lhs.name.get_string(), rhs.name.get_string()) < 0);
    }
};

int main() {
    Document d(kObjectType);
    Document::AllocatorType &allocator = d.get_allocator();

    d.add_member("zeta", Value().SetBool(false), allocator);
    d.add_member("gama", Value().set_string("test string", allocator), allocator);
    d.add_member("delta", Value().set_int32(123), allocator);
    d.add_member("alpha", Value(kArrayType).Move(), allocator);

    printIt(d);

/*
{
    "zeta": false,
    "gama": "test string",
    "delta": 123,
    "alpha": []
}
*/

// C++11 supports std::move() of Value so it always have no problem for std::sort().
// Some C++03 implementations of std::sort() requires copy constructor which causes compilation error.
// Needs a sorting function only depends on std::swap() instead.
#if __cplusplus >= 201103L || (!defined(__GLIBCXX__) && (!defined(_MSC_VER) || _MSC_VER >= 1900))
    std::sort(d.member_begin(), d.member_end(), NameComparator());

    printIt(d);

/*
{
  "alpha": [],
  "delta": 123,
  "gama": "test string",
  "zeta": false
}
*/
#endif
}
