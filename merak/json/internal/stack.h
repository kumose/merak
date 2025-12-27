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

#ifndef MERAK_JSON_INTERNAL_STACK_H_
#define MERAK_JSON_INTERNAL_STACK_H_

#include <merak/json/allocators.h>
#include <merak/json/internal/swap.h>
#include <cstddef>

#if defined(__clang__)
MERAK_JSON_DIAG_PUSH
MERAK_JSON_DIAG_OFF(c++98-compat)
#endif

namespace merak::json {
namespace internal {

///////////////////////////////////////////////////////////////////////////////
// Stack

//! A type-unsafe stack for storing different types of data.
/*! \tparam Allocator Allocator for allocating stack memory.
*/
template <typename Allocator>
class Stack {
public:
    // Optimization note: Do not allocate memory for stack_ in constructor.
    // Do it lazily when first Push() -> Expand() -> Resize().
    Stack(Allocator* allocator, size_t stackCapacity) : allocator_(allocator), ownAllocator_(0), stack_(0), stackTop_(0), stackEnd_(0), initialCapacity_(stackCapacity) {
    }

    Stack(Stack&& rhs)
        : allocator_(rhs.allocator_),
          ownAllocator_(rhs.ownAllocator_),
          stack_(rhs.stack_),
          stackTop_(rhs.stackTop_),
          stackEnd_(rhs.stackEnd_),
          initialCapacity_(rhs.initialCapacity_)
    {
        rhs.allocator_ = 0;
        rhs.ownAllocator_ = 0;
        rhs.stack_ = 0;
        rhs.stackTop_ = 0;
        rhs.stackEnd_ = 0;
        rhs.initialCapacity_ = 0;
    }

    ~Stack() {
        destroy();
    }

    Stack& operator=(Stack&& rhs) {
        if (&rhs != this)
        {
            destroy();

            allocator_ = rhs.allocator_;
            ownAllocator_ = rhs.ownAllocator_;
            stack_ = rhs.stack_;
            stackTop_ = rhs.stackTop_;
            stackEnd_ = rhs.stackEnd_;
            initialCapacity_ = rhs.initialCapacity_;

            rhs.allocator_ = 0;
            rhs.ownAllocator_ = 0;
            rhs.stack_ = 0;
            rhs.stackTop_ = 0;
            rhs.stackEnd_ = 0;
            rhs.initialCapacity_ = 0;
        }
        return *this;
    }

    void swap(Stack& rhs) MERAK_JSON_NOEXCEPT {
        internal::swap(allocator_, rhs.allocator_);
        internal::swap(ownAllocator_, rhs.ownAllocator_);
        internal::swap(stack_, rhs.stack_);
        internal::swap(stackTop_, rhs.stackTop_);
        internal::swap(stackEnd_, rhs.stackEnd_);
        internal::swap(initialCapacity_, rhs.initialCapacity_);
    }

    void clear() { stackTop_ = stack_; }

    void ShrinkToFit() { 
        if (empty()) {
            // If the stack is empty, completely deallocate the memory.
            Allocator::Free(stack_); // NOLINT (+clang-analyzer-unix.Malloc)
            stack_ = 0;
            stackTop_ = 0;
            stackEnd_ = 0;
        }
        else
            Resize(GetSize());
    }

    // Optimization note: try to minimize the size of this function for force inline.
    // Expansion is run very infrequently, so it is moved to another (probably non-inline) function.
    template<typename T>
    MERAK_JSON_FORCEINLINE void reserve(size_t count = 1) {
         // Expand the stack if needed
        if (MERAK_JSON_UNLIKELY(static_cast<std::ptrdiff_t>(sizeof(T) * count) > (stackEnd_ - stackTop_)))
            Expand<T>(count);
    }

    template<typename T>
    MERAK_JSON_FORCEINLINE T* Push(size_t count = 1) {
        reserve<T>(count);
        return PushUnsafe<T>(count);
    }

    template<typename T>
    MERAK_JSON_FORCEINLINE T* PushUnsafe(size_t count = 1) {
        MERAK_JSON_ASSERT(stackTop_);
        MERAK_JSON_ASSERT(static_cast<std::ptrdiff_t>(sizeof(T) * count) <= (stackEnd_ - stackTop_));
        T* ret = reinterpret_cast<T*>(stackTop_);
        stackTop_ += sizeof(T) * count;
        return ret;
    }

    template<typename T>
    T* Pop(size_t count) {
        MERAK_JSON_ASSERT(GetSize() >= count * sizeof(T));
        stackTop_ -= count * sizeof(T);
        return reinterpret_cast<T*>(stackTop_);
    }

    template<typename T>
    T* Top() { 
        MERAK_JSON_ASSERT(GetSize() >= sizeof(T));
        return reinterpret_cast<T*>(stackTop_ - sizeof(T));
    }

    template<typename T>
    const T* Top() const {
        MERAK_JSON_ASSERT(GetSize() >= sizeof(T));
        return reinterpret_cast<T*>(stackTop_ - sizeof(T));
    }

    template<typename T>
    T* end() { return reinterpret_cast<T*>(stackTop_); }

    template<typename T>
    const T* end() const { return reinterpret_cast<T*>(stackTop_); }

    template<typename T>
    T* Bottom() { return reinterpret_cast<T*>(stack_); }

    template<typename T>
    const T* Bottom() const { return reinterpret_cast<T*>(stack_); }

    bool HasAllocator() const {
        return allocator_ != 0;
    }

    Allocator& get_allocator() {
        MERAK_JSON_ASSERT(allocator_);
        return *allocator_;
    }

    bool empty() const { return stackTop_ == stack_; }
    size_t GetSize() const { return static_cast<size_t>(stackTop_ - stack_); }
    size_t GetCapacity() const { return static_cast<size_t>(stackEnd_ - stack_); }

private:
    template<typename T>
    void Expand(size_t count) {
        // Only expand the capacity if the current stack exists. Otherwise just create a stack with initial capacity.
        size_t newCapacity;
        if (stack_ == 0) {
            if (!allocator_)
                ownAllocator_ = allocator_ = MERAK_JSON_NEW(Allocator)();
            newCapacity = initialCapacity_;
        } else {
            newCapacity = GetCapacity();
            newCapacity += (newCapacity + 1) / 2;
        }
        size_t newSize = GetSize() + sizeof(T) * count;
        if (newCapacity < newSize)
            newCapacity = newSize;

        Resize(newCapacity);
    }

    void Resize(size_t newCapacity) {
        const size_t size = GetSize();  // Backup the current size
        stack_ = static_cast<char*>(allocator_->Realloc(stack_, GetCapacity(), newCapacity));
        stackTop_ = stack_ + size;
        stackEnd_ = stack_ + newCapacity;
    }

    void destroy() {
        Allocator::Free(stack_);
        MERAK_JSON_DELETE(ownAllocator_); // Only delete if it is owned by the stack
    }

    // Prohibit copy constructor & assignment operator.
    Stack(const Stack&);
    Stack& operator=(const Stack&);

    Allocator* allocator_;
    Allocator* ownAllocator_;
    char *stack_;
    char *stackTop_;
    char *stackEnd_;
    size_t initialCapacity_;
};

} // namespace internal
}  // namespace merak::json

#if defined(__clang__)
MERAK_JSON_DIAG_POP
#endif

#endif // MERAK_JSON_STACK_H_
