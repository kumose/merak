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
