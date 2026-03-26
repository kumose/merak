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

#ifndef MERAK_JSON_INTERNAL_STRFUNC_H_
#define MERAK_JSON_INTERNAL_STRFUNC_H_

#include <merak/json/stream.h>
#include <cwchar>

namespace merak::json {
namespace internal {

//! Custom strlen() which works on different character types.
/*! \tparam char_type Character type (e.g. char, wchar_t, short)
    \param s Null-terminated input string.
    \return Number of characters in the string. 
    \note This has the same semantics as strlen(), the return value is not number of Unicode codepoints.
*/
template <typename char_type>
inline SizeType StrLen(const char_type* s) {
    MERAK_JSON_ASSERT(s != 0);
    const char_type* p = s;
    while (*p) ++p;
    return SizeType(p - s);
}

template <>
inline SizeType StrLen(const char* s) {
    return SizeType(std::strlen(s));
}

template <>
inline SizeType StrLen(const wchar_t* s) {
    return SizeType(std::wcslen(s));
}

//! Custom strcmpn() which works on different character types.
/*! \tparam char_type Character type (e.g. char, wchar_t, short)
    \param s1 Null-terminated input string.
    \param s2 Null-terminated input string.
    \return 0 if equal
*/
template<typename char_type>
inline int StrCmp(const char_type* s1, const char_type* s2) {
    MERAK_JSON_ASSERT(s1 != 0);
    MERAK_JSON_ASSERT(s2 != 0);
    while(*s1 && (*s1 == *s2)) { s1++; s2++; }
    return static_cast<unsigned>(*s1) < static_cast<unsigned>(*s2) ? -1 : static_cast<unsigned>(*s1) > static_cast<unsigned>(*s2);
}

//! Returns number of code points in a encoded string.
template<typename Encoding>
bool CountStringCodePoint(const typename Encoding::char_type* s, SizeType length, SizeType* outCount) {
    MERAK_JSON_ASSERT(s != 0);
    MERAK_JSON_ASSERT(outCount != 0);
    GenericStringStream<Encoding> is(s);
    const typename Encoding::char_type* end = s + length;
    SizeType count = 0;
    while (is.src_ < end) {
        unsigned codepoint;
        if (!Encoding::Decode(is, &codepoint))
            return false;
        count++;
    }
    *outCount = count;
    return true;
}

} // namespace internal
}  // namespace merak::json

#endif // MERAK_JSON_INTERNAL_STRFUNC_H_
