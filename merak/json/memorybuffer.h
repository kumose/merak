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
