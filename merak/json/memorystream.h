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

#ifndef MERAK_JSON_MEMORYSTREAM_H_
#define MERAK_JSON_MEMORYSTREAM_H_

#include <merak/json/stream.h>

#ifdef __clang__
MERAK_JSON_DIAG_PUSH
MERAK_JSON_DIAG_OFF(unreachable-code)
MERAK_JSON_DIAG_OFF(missing-noreturn)
#endif

namespace merak::json {

//! Represents an in-memory input byte stream.
/*!
    This class is mainly for being wrapped by EncodedInputStream.

    It is similar to FileReadBuffer but the source is an in-memory buffer instead of a file.

    Differences between MemoryStream and StringStream:
    1. StringStream has encoding but MemoryStream is a byte stream.
    2. MemoryStream needs size of the source buffer and the buffer don't need to be null terminated. StringStream assume null-terminated string as source.
    \note implements Stream concept
*/
struct MemoryStream {
    typedef char char_type; // byte

    MemoryStream(const char_type *src, size_t size) : src_(src), begin_(src), end_(src + size), size_(size) {}

    char_type peek() const { return MERAK_JSON_UNLIKELY(src_ == end_) ? '\0' : *src_; }
    char_type get() { return MERAK_JSON_UNLIKELY(src_ == end_) ? '\0' : *src_++; }
    size_t tellg() const { return static_cast<size_t>(src_ - begin_); }

    const char_type* src_;     //!< Current read position.
    const char_type* begin_;   //!< Original head of the string.
    const char_type* end_;     //!< end of stream.
    size_t size_;       //!< size of the stream.
};

}  // namespace merak::json

#ifdef __clang__
MERAK_JSON_DIAG_POP
#endif

#endif // MERAK_JSON_MEMORYBUFFER_H_
