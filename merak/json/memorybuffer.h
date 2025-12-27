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

#ifndef MERAK_JSON_MEMORYBUFFER_H_
#define MERAK_JSON_MEMORYBUFFER_H_

#include <merak/json/stream.h>
#include <merak/json/internal/stack.h>

namespace merak::json {

//! Represents an in-memory output byte stream.
/*!
    This class is mainly for being wrapped by EncodedOutputStream or AutoUTFOutputStream.

    It is similar to FileWriteBuffer but the destination is an in-memory buffer instead of a file.

    Differences between MemoryBuffer and StringBuffer:
    1. StringBuffer has Encoding but MemoryBuffer is only a byte buffer. 
    2. StringBuffer::get_string() returns a null-terminated string. MemoryBuffer::GetBuffer() returns a buffer without terminator.

    \tparam Allocator type for allocating memory buffer.
    \note implements Stream concept
*/
template <typename Allocator = CrtAllocator>
struct GenericMemoryBuffer {
    typedef char char_type; // byte

    GenericMemoryBuffer(Allocator* allocator = 0, size_t capacity = kDefaultCapacity) : stack_(allocator, capacity) {}

    void put(char_type c) { *stack_.template Push<char_type>() = c; }
    GenericMemoryBuffer &flush() {
        return *this;
    }

    void clear() { stack_.clear(); }
    void ShrinkToFit() { stack_.ShrinkToFit(); }
    char_type* Push(size_t count) { return stack_.template Push<char_type>(count); }
    void Pop(size_t count) { stack_.template Pop<char_type>(count); }

    const char_type* GetBuffer() const {
        return stack_.template Bottom<char_type>();
    }

    size_t GetSize() const { return stack_.GetSize(); }

    static const size_t kDefaultCapacity = 256;
    mutable internal::Stack<Allocator> stack_;
};

typedef GenericMemoryBuffer<> MemoryBuffer;

//! Implement specialized version of PutN() with memset() for better performance.
template<>
inline void PutN(MemoryBuffer& memoryBuffer, char c, size_t n) {
    std::memset(memoryBuffer.stack_.Push<char>(n), c, n * sizeof(c));
}

}  // namespace merak::json

#endif // MERAK_JSON_MEMORYBUFFER_H_
