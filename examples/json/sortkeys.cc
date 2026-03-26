
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
