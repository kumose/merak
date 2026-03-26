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

#ifndef MERAK_JSON_STRINGBUFFER_H_
#define MERAK_JSON_STRINGBUFFER_H_

#include <merak/json/stream.h>
#include <merak/json/internal/stack.h>
#include <utility> // std::move
#include <merak/json/internal/stack.h>

#if defined(__clang__)
MERAK_JSON_DIAG_PUSH
MERAK_JSON_DIAG_OFF(c++98-compat)
#endif

namespace merak::json {

    //! Represents an in-memory output stream.
    /*!
        \tparam Encoding Encoding of the stream.
        \tparam Allocator type for allocating memory buffer.
        \note implements Stream concept
    */
    template<typename Encoding, typename Allocator = CrtAllocator>
    class GenericStringBuffer {
    public:
        typedef typename Encoding::char_type char_type;

        GenericStringBuffer(Allocator *allocator = 0, size_t capacity = kDefaultCapacity) : stack_(allocator,
                                                                                                   capacity) {}


        GenericStringBuffer(GenericStringBuffer &&rhs) : stack_(std::move(rhs.stack_)) {}

        GenericStringBuffer &operator=(GenericStringBuffer &&rhs) {
            if (&rhs != this)
                stack_ = std::move(rhs.stack_);
            return *this;
        }


        void put(char_type c) { *stack_.template Push<char_type>() = c; }

        void write(const char_type* c, size_t length) {
            char_type* pos = stack_.template Push<char_type>(length);
            memcpy(pos, c, length * sizeof(char_type));
        }

        void PutUnsafe(char_type c) { *stack_.template PushUnsafe<char_type>() = c; }

        GenericStringBuffer &flush() {
            return *this;
        }

        void clear() { stack_.clear(); }

        void ShrinkToFit() {
            // Push and pop a null terminator. This is safe.
            *stack_.template Push<char_type>() = '\0';
            stack_.ShrinkToFit();
            stack_.template Pop<char_type>(1);
        }

        void reserve(size_t count) { stack_.template reserve<char_type>(count); }

        char_type *Push(size_t count) { return stack_.template Push<char_type>(count); }

        char_type *PushUnsafe(size_t count) { return stack_.template PushUnsafe<char_type>(count); }

        void Pop(size_t count) { stack_.template Pop<char_type>(count); }

        const char_type *get_string() const {
            // Push and pop a null terminator. This is safe.
            *stack_.template Push<char_type>() = '\0';
            stack_.template Pop<char_type>(1);

            return stack_.template Bottom<char_type>();
        }

        //! Get the size of string in bytes in the string buffer.
        size_t GetSize() const { return stack_.GetSize(); }

        //! Get the length of string in char_type in the string buffer.
        size_t get_length() const { return stack_.GetSize() / sizeof(char_type); }

        static const size_t kDefaultCapacity = 256;
        mutable internal::Stack<Allocator> stack_;

    private:
        // Prohibit copy constructor & assignment operator.
        GenericStringBuffer(const GenericStringBuffer &);

        GenericStringBuffer &operator=(const GenericStringBuffer &);
    };

    //! String buffer with UTF8 encoding
    typedef GenericStringBuffer<UTF8<> > StringBuffer;

    template<typename Encoding, typename Allocator>
    inline void PutReserve(GenericStringBuffer<Encoding, Allocator> &stream, size_t count) {
        stream.reserve(count);
    }

    template<typename Encoding, typename Allocator>
    inline void PutUnsafe(GenericStringBuffer<Encoding, Allocator> &stream, typename Encoding::char_type c) {
        stream.PutUnsafe(c);
    }

    //! Implement specialized version of PutN() with memset() for better performance.
    template<>
    inline void PutN(GenericStringBuffer<UTF8<> > &stream, char c, size_t n) {
        std::memset(stream.stack_.Push<char>(n), c, n * sizeof(c));
    }

}  // namespace merak::json

#if defined(__clang__)
MERAK_JSON_DIAG_POP
#endif

#endif // MERAK_JSON_STRINGBUFFER_H_
