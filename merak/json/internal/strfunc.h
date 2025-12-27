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
