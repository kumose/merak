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


#ifndef MERAK_JSON_DOCUMENT_H_
#define MERAK_JSON_DOCUMENT_H_

/*! \file document.h */

#include <merak/json/reader.h>
#include <merak/json/internal/meta.h>
#include <merak/json/internal/strfunc.h>
#include <merak/json/memorystream.h>
#include <merak/json/encodedstream.h>
#include <merak/json/std_input_stream.h>
#include <merak/json/std_output_stream.h>
#include <new>      // placement new
#include <limits>

#ifdef __cpp_lib_three_way_comparison
#include <compare>
#endif

MERAK_JSON_DIAG_PUSH
#ifdef __clang__
MERAK_JSON_DIAG_OFF(padded)
MERAK_JSON_DIAG_OFF(switch-enum)
MERAK_JSON_DIAG_OFF(c++98-compat)
#elif defined(_MSC_VER)
MERAK_JSON_DIAG_OFF(4127) // conditional expression is constant
MERAK_JSON_DIAG_OFF(4244) // conversion from kXxxFlags to 'uint16_t', possible loss of data
#endif

#ifdef __GNUC__
MERAK_JSON_DIAG_OFF(effc++)
#endif // __GNUC__

#ifndef MERAK_JSON_NOMEMBERITERATORCLASS

#include <iterator> // std::random_access_iterator_tag

#endif

#if MERAK_JSON_USE_MEMBERSMAP
#include <map> // std::multimap
#endif

namespace merak::json {

    // Forward declaration.
    template<typename Encoding, typename Allocator>
    class GenericValue;

    template<typename Encoding, typename Allocator, typename StackAllocator>
    class GenericDocument;

    /*! \def MERAK_JSON_DEFAULT_ALLOCATOR
        \ingroup MERAK_JSON_CONFIG
        \brief Allows to choose default allocator.

        User can define this to use CrtAllocator or MemoryPoolAllocator.
    */
#ifndef MERAK_JSON_DEFAULT_ALLOCATOR
#define MERAK_JSON_DEFAULT_ALLOCATOR ::merak::json::MemoryPoolAllocator<::merak::json::CrtAllocator>
#endif

    /*! \def MERAK_JSON_DEFAULT_STACK_ALLOCATOR
        \ingroup MERAK_JSON_CONFIG
        \brief Allows to choose default stack allocator for Document.

        User can define this to use CrtAllocator or MemoryPoolAllocator.
    */
#ifndef MERAK_JSON_DEFAULT_STACK_ALLOCATOR
#define MERAK_JSON_DEFAULT_STACK_ALLOCATOR ::merak::json::CrtAllocator
#endif

    /*! \def MERAK_JSON_VALUE_DEFAULT_OBJECT_CAPACITY
        \ingroup MERAK_JSON_CONFIG
        \brief User defined kDefaultObjectCapacity value.

        User can define this as any natural number.
    */
#ifndef MERAK_JSON_VALUE_DEFAULT_OBJECT_CAPACITY
    // number of objects that merak::json::Value allocates memory for by default
#define MERAK_JSON_VALUE_DEFAULT_OBJECT_CAPACITY 16
#endif

    /*! \def MERAK_JSON_VALUE_DEFAULT_ARRAY_CAPACITY
        \ingroup MERAK_JSON_CONFIG
        \brief User defined kDefaultArrayCapacity value.

        User can define this as any natural number.
    */
#ifndef MERAK_JSON_VALUE_DEFAULT_ARRAY_CAPACITY
    // number of array elements that merak::json::Value allocates memory for by default
#define MERAK_JSON_VALUE_DEFAULT_ARRAY_CAPACITY 16
#endif

    //! Name-value pair in a JSON object value.
    /*!
        This class was internal to GenericValue. It used to be a inner struct.
        But a compiler (IBM XL C/C++ for AIX) have reported to have problem with that so it moved as a namespace scope struct.
        https://code.google.com/p/rapidjson/issues/detail?id=64
    */
    template<typename Encoding, typename Allocator>
    class GenericMember {
    public:
        GenericValue<Encoding, Allocator> name;     //!< name of member (must be a string)
        GenericValue<Encoding, Allocator> value;    //!< value of member.


        //! Move constructor in C++11
        GenericMember(GenericMember &&rhs) MERAK_JSON_NOEXCEPT
                : name(std::move(rhs.name)),
                  value(std::move(rhs.value)) {
        }

        //! Move assignment in C++11
        GenericMember &operator=(GenericMember &&rhs) MERAK_JSON_NOEXCEPT {
            return *this = static_cast<GenericMember &>(rhs);
        }


        //! Assignment with move semantics.
        /*! \param rhs Source of the assignment. Its name and value will become a null value after assignment.
        */
        GenericMember &operator=(GenericMember &rhs) MERAK_JSON_NOEXCEPT {
            if (MERAK_JSON_LIKELY(this != &rhs)) {
                name = rhs.name;
                value = rhs.value;
            }
            return *this;
        }

        // swap() for std::sort() and other potential use in STL.
        friend inline void swap(GenericMember &a, GenericMember &b) MERAK_JSON_NOEXCEPT {
            a.name.swap(b.name);
            a.value.swap(b.value);
        }

    private:
        //! Copy constructor is not permitted.
        GenericMember(const GenericMember &rhs);
    };

    ///////////////////////////////////////////////////////////////////////////////
    // GenericMemberIterator

#ifndef MERAK_JSON_NOMEMBERITERATORCLASS

    //! (Constant) member iterator for a JSON object value
    /*!
        \tparam Const is_type this a constant iterator?
        \tparam Encoding    Encoding of the value. (Even non-string values need to have the same encoding in a document)
        \tparam Allocator   Allocator type for allocating memory of object, array and string.

        This class implements a Random Access Iterator for GenericMember elements
        of a GenericValue, see ISO/IEC 14882:2003(E) C++ standard, 24.1 [lib.iterator.requirements].

        \note This iterator implementation is mainly intended to avoid implicit
            conversions from iterator values to \c NULL,
            e.g. from GenericValue::find_member.

        \note Define \c MERAK_JSON_NOMEMBERITERATORCLASS to fall back to a
            pointer-based implementation, if your platform doesn't provide
            the C++ <iterator> header.

        \see GenericMember, GenericValue::MemberIterator, GenericValue::ConstMemberIterator
     */
    template<bool Const, typename Encoding, typename Allocator>
    class GenericMemberIterator {

        friend class GenericValue<Encoding, Allocator>;

        template<bool, typename, typename> friend
        class GenericMemberIterator;

        typedef GenericMember<Encoding, Allocator> PlainType;
        typedef typename internal::MaybeAddConst<Const, PlainType>::Type ValueType;

    public:
        //! Iterator type itself
        typedef GenericMemberIterator Iterator;
        //! Constant iterator type
        typedef GenericMemberIterator<true, Encoding, Allocator> ConstIterator;
        //! Non-constant iterator type
        typedef GenericMemberIterator<false, Encoding, Allocator> NonConstIterator;

        /** \name std::iterator_traits support */
        //@{
        typedef ValueType value_type;
        typedef ValueType *pointer;
        typedef ValueType &reference;
        typedef std::ptrdiff_t difference_type;
        typedef std::random_access_iterator_tag iterator_category;
        //@}

        //! Pointer to (const) GenericMember
        typedef pointer Pointer;
        //! Reference to (const) GenericMember
        typedef reference Reference;
        //! Signed integer type (e.g. \c ptrdiff_t)
        typedef difference_type DifferenceType;

        //! Default constructor (singular value)
        /*! Creates an iterator pointing to no element.
            \note All operations, except for comparisons, are undefined on such values.
         */
        GenericMemberIterator() : ptr_() {}

        //! Iterator conversions to more const
        /*!
            \param it (Non-const) iterator to copy from

            Allows the creation of an iterator from another GenericMemberIterator
            that is "less const".  Especially, creating a non-constant iterator
            from a constant iterator are disabled:
            \li const -> non-const (not ok)
            \li const -> const (ok)
            \li non-const -> const (ok)
            \li non-const -> non-const (ok)

            \note If the \c Const template parameter is already \c false, this
                constructor effectively defines a regular copy-constructor.
                Otherwise, the copy constructor is implicitly defined.
        */
        GenericMemberIterator(const NonConstIterator &it) : ptr_(it.ptr_) {}

        Iterator &operator=(const NonConstIterator &it) {
            ptr_ = it.ptr_;
            return *this;
        }

        //! @name stepping
        //@{
        Iterator &operator++() {
            ++ptr_;
            return *this;
        }

        Iterator &operator--() {
            --ptr_;
            return *this;
        }

        Iterator operator++(int) {
            Iterator old(*this);
            ++ptr_;
            return old;
        }

        Iterator operator--(int) {
            Iterator old(*this);
            --ptr_;
            return old;
        }
        //@}

        //! @name increment/decrement
        //@{
        Iterator operator+(DifferenceType n) const { return Iterator(ptr_ + n); }

        Iterator operator-(DifferenceType n) const { return Iterator(ptr_ - n); }

        Iterator &operator+=(DifferenceType n) {
            ptr_ += n;
            return *this;
        }

        Iterator &operator-=(DifferenceType n) {
            ptr_ -= n;
            return *this;
        }
        //@}

        //! @name relations
        //@{
        template<bool Const_>
        bool operator==(const GenericMemberIterator<Const_, Encoding, Allocator> &that) const {
            return ptr_ == that.ptr_;
        }

        template<bool Const_>
        bool operator!=(const GenericMemberIterator<Const_, Encoding, Allocator> &that) const {
            return ptr_ != that.ptr_;
        }

        template<bool Const_>
        bool operator<=(const GenericMemberIterator<Const_, Encoding, Allocator> &that) const {
            return ptr_ <= that.ptr_;
        }

        template<bool Const_>
        bool operator>=(const GenericMemberIterator<Const_, Encoding, Allocator> &that) const {
            return ptr_ >= that.ptr_;
        }

        template<bool Const_>
        bool operator<(const GenericMemberIterator<Const_, Encoding, Allocator> &that) const {
            return ptr_ < that.ptr_;
        }

        template<bool Const_>
        bool operator>(const GenericMemberIterator<Const_, Encoding, Allocator> &that) const {
            return ptr_ > that.ptr_;
        }

#ifdef __cpp_lib_three_way_comparison
        template <bool Const_> std::strong_ordering operator<=>(const GenericMemberIterator<Const_, Encoding, Allocator>& that) const { return ptr_ <=> that.ptr_; }
#endif
        //@}

        //! @name dereference
        //@{
        Reference operator*() const { return *ptr_; }

        Pointer operator->() const { return ptr_; }

        Reference operator[](DifferenceType n) const { return ptr_[n]; }
        //@}

        //! Distance
        DifferenceType operator-(ConstIterator that) const { return ptr_ - that.ptr_; }

    private:
        //! Internal constructor from plain pointer
        explicit GenericMemberIterator(Pointer p) : ptr_(p) {}

        Pointer ptr_; //!< raw pointer
    };

#else // MERAK_JSON_NOMEMBERITERATORCLASS

    // class-based member iterator implementation disabled, use plain pointers

    template <bool Const, typename Encoding, typename Allocator>
    class GenericMemberIterator;

    //! non-const GenericMemberIterator
    template <typename Encoding, typename Allocator>
    class GenericMemberIterator<false,Encoding,Allocator> {
    public:
        //! use plain pointer as iterator type
        typedef GenericMember<Encoding,Allocator>* Iterator;
    };
    //! const GenericMemberIterator
    template <typename Encoding, typename Allocator>
    class GenericMemberIterator<true,Encoding,Allocator> {
    public:
        //! use plain const pointer as iterator type
        typedef const GenericMember<Encoding,Allocator>* Iterator;
    };

#endif // MERAK_JSON_NOMEMBERITERATORCLASS

    ///////////////////////////////////////////////////////////////////////////////
    // GenericStringRef

    //! Reference to a constant string (not taking a copy)
    /*!
        \tparam CharType character type of the string

        This helper class is used to automatically infer constant string
        references for string literals, especially from \c const \b (!)
        character arrays.

        The main use is for creating JSON string values without copying the
        source string via an \ref Allocator.  This requires that the referenced
        string pointers have a sufficient lifetime, which exceeds the lifetime
        of the associated GenericValue.

        \b Example
        \code
        Value v("foo");   // ok, no need to copy & calculate length
        const char foo[] = "foo";
        v.set_string(foo); // ok

        const char* bar = foo;
        // Value x(bar); // not ok, can't rely on bar's lifetime
        Value x(StringRef(bar)); // lifetime explicitly guaranteed by user
        Value y(StringRef(bar, 3));  // ok, explicitly pass length
        \endcode

        \see StringRef, GenericValue::set_string
    */
    template<typename CharType>
    struct GenericStringRef {
        typedef CharType char_type; //!< character type of the string

        //! Create string reference from \c const character array
#ifndef __clang__ // -Wdocumentation
        /*!
            This constructor implicitly creates a constant string reference from
            a \c const character array.  It has better performance than
            \ref StringRef(const CharType*) by inferring the string \ref length
            from the array length, and also supports strings containing null
            characters.

            \tparam N length of the string, automatically inferred

            \param str Constant character array, lifetime assumed to be longer
                than the use of the string in e.g. a GenericValue

            \post \ref s == str

            \note Constant complexity.
            \note There is a hidden, private overload to disallow references to
                non-const character arrays to be created via this constructor.
                By this, e.g. function-scope arrays used to be filled via
                \c snprintf are excluded from consideration.
                In such cases, the referenced string should be \b copied to the
                GenericValue instead.
         */
#endif

        template<SizeType N>
        GenericStringRef(const CharType (&str)[N]) MERAK_JSON_NOEXCEPT
                : s(str), length(N - 1) {}

        //! Explicitly create string reference from \c const character pointer
#ifndef __clang__ // -Wdocumentation
        /*!
            This constructor can be used to \b explicitly  create a reference to
            a constant string pointer.

            \see StringRef(const CharType*)

            \param str Constant character pointer, lifetime assumed to be longer
                than the use of the string in e.g. a GenericValue

            \post \ref s == str

            \note There is a hidden, private overload to disallow references to
                non-const character arrays to be created via this constructor.
                By this, e.g. function-scope arrays used to be filled via
                \c snprintf are excluded from consideration.
                In such cases, the referenced string should be \b copied to the
                GenericValue instead.
         */
#endif

        explicit GenericStringRef(const CharType *str)
                : s(str), length(NotNullStrLen(str)) {}

        //! Create constant string reference from pointer and length
#ifndef __clang__ // -Wdocumentation
        /*! \param str constant string, lifetime assumed to be longer than the use of the string in e.g. a GenericValue
            \param len length of the string, excluding the trailing NULL terminator

            \post \ref s == str && \ref length == len
            \note Constant complexity.
         */
#endif

        GenericStringRef(const CharType *str, SizeType len)
                : s(MERAK_JSON_LIKELY(str) ? str : emptyString), length(len) {
            MERAK_JSON_ASSERT(str != 0 || len == 0u);
        }

        GenericStringRef(const GenericStringRef &rhs) : s(rhs.s), length(rhs.length) {}

        //! implicit conversion to plain CharType pointer
        operator const char_type *() const { return s; }

        const char_type *const s; //!< plain CharType pointer
        const SizeType length; //!< length of the string (excluding the trailing NULL terminator)

    private:
        SizeType NotNullStrLen(const CharType *str) {
            MERAK_JSON_ASSERT(str != 0);
            return internal::StrLen(str);
        }

        /// empty string - used when passing in a NULL pointer
        static const char_type emptyString[];

        //! Disallow construction from non-const array
        template<SizeType N>
        GenericStringRef(CharType (&str)[N]) /* = delete */;

        //! Copy assignment operator not permitted - immutable type
        GenericStringRef &operator=(const GenericStringRef &rhs) /* = delete */;
    };

    template<typename CharType>
    const CharType GenericStringRef<CharType>::emptyString[] = {CharType()};

    //! Mark a character pointer as constant string
    /*! Mark a plain character pointer as a "string literal".  This function
        can be used to avoid copying a character string to be referenced as a
        value in a JSON GenericValue object, if the string's lifetime is known
        to be valid long enough.
        \tparam CharType Character type of the string
        \param str Constant string, lifetime assumed to be longer than the use of the string in e.g. a GenericValue
        \return GenericStringRef string reference object
        \relatesalso GenericStringRef

        \see GenericValue::GenericValue(StringRefType), GenericValue::operator=(StringRefType), GenericValue::set_string(StringRefType), GenericValue::push_back(StringRefType, Allocator&), GenericValue::add_member
    */
    template<typename CharType>
    inline GenericStringRef<CharType> StringRef(const CharType *str) {
        return GenericStringRef<CharType>(str);
    }

    //! Mark a character pointer as constant string
    /*! Mark a plain character pointer as a "string literal".  This function
        can be used to avoid copying a character string to be referenced as a
        value in a JSON GenericValue object, if the string's lifetime is known
        to be valid long enough.

        This version has better performance with supplied length, and also
        supports string containing null characters.

        \tparam CharType character type of the string
        \param str Constant string, lifetime assumed to be longer than the use of the string in e.g. a GenericValue
        \param length The length of source string.
        \return GenericStringRef string reference object
        \relatesalso GenericStringRef
    */
    template<typename CharType>
    inline GenericStringRef<CharType> StringRef(const CharType *str, size_t length) {
        return GenericStringRef<CharType>(str, SizeType(length));
    }

    //! Mark a string object as constant string
    /*! Mark a string object (e.g. \c std::string) as a "string literal".
        This function can be used to avoid copying a string to be referenced as a
        value in a JSON GenericValue object, if the string's lifetime is known
        to be valid long enough.

        \tparam CharType character type of the string
        \param str Constant string, lifetime assumed to be longer than the use of the string in e.g. a GenericValue
        \return GenericStringRef string reference object
        \relatesalso GenericStringRef
        \note Requires the definition of the preprocessor symbol.
    */
    template<typename CharType>
    inline GenericStringRef<CharType> StringRef(const std::basic_string<CharType> &str) {
        return GenericStringRef<CharType>(str.data(), SizeType(str.size()));
    }

    ///////////////////////////////////////////////////////////////////////////////
    // GenericValue type traits
    namespace internal {

        template<typename T, typename Encoding = void, typename Allocator = void>
        struct IsGenericValueImpl : FalseType {
        };

        // select candidates according to nested encoding and allocator types
        template<typename T>
        struct IsGenericValueImpl<T, typename Void<typename T::EncodingType>::Type, typename Void<typename T::AllocatorType>::Type>
                : IsBaseOf<GenericValue<typename T::EncodingType, typename T::AllocatorType>, T>::Type {
        };

        // helper to match arbitrary GenericValue instantiations, including derived classes
        template<typename T>
        struct IsGenericValue : IsGenericValueImpl<T>::Type {
        };

    } // namespace internal

    ///////////////////////////////////////////////////////////////////////////////
    // TypeHelper

    namespace internal {

        template<typename ValueType, typename T>
        struct TypeHelper {
        };

        template<typename ValueType>
        struct TypeHelper<ValueType, bool> {
            static bool is_type(const ValueType &v) { return v.is_bool(); }

            static bool get(const ValueType &v) { return v.get_bool(); }

            static ValueType &set(ValueType &v, bool data) { return v.SetBool(data); }

            static ValueType &set(ValueType &v, bool data, typename ValueType::AllocatorType &) {
                return v.SetBool(data);
            }
        };

        template<typename ValueType>
        struct TypeHelper<ValueType, int> {
            static bool is_type(const ValueType &v) { return v.is_int32(); }

            static int get(const ValueType &v) { return v.get_int32(); }

            static ValueType &set(ValueType &v, int data) { return v.set_int32(data); }

            static ValueType &set(ValueType &v, int data, typename ValueType::AllocatorType &) {
                return v.set_int32(data);
            }
        };

        template<typename ValueType>
        struct TypeHelper<ValueType, unsigned> {
            static bool is_type(const ValueType &v) { return v.is_uint32(); }

            static unsigned get(const ValueType &v) { return v.get_uint32(); }

            static ValueType &set(ValueType &v, unsigned data) { return v.set_uint32(data); }

            static ValueType &set(ValueType &v, unsigned data, typename ValueType::AllocatorType &) {
                return v.set_uint32(data);
            }
        };

#ifdef _MSC_VER
        MERAK_JSON_STATIC_ASSERT(sizeof(long) == sizeof(int));
        template<typename ValueType>
        struct TypeHelper<ValueType, long> {
            static bool is_type(const ValueType& v) { return v.is_int32(); }
            static long get(const ValueType& v) { return v.get_int32(); }
            static ValueType& set(ValueType& v, long data) { return v.set_int32(data); }
            static ValueType& set(ValueType& v, long data, typename ValueType::AllocatorType&) { return v.set_int32(data); }
        };

        MERAK_JSON_STATIC_ASSERT(sizeof(unsigned long) == sizeof(unsigned));
        template<typename ValueType>
        struct TypeHelper<ValueType, unsigned long> {
            static bool is_type(const ValueType& v) { return v.is_uint32(); }
            static unsigned long get(const ValueType& v) { return v.get_uint32(); }
            static ValueType& set(ValueType& v, unsigned long data) { return v.set_uint32(data); }
            static ValueType& set(ValueType& v, unsigned long data, typename ValueType::AllocatorType&) { return v.set_uint32(data); }
        };
#endif

        template<typename ValueType>
        struct TypeHelper<ValueType, int64_t> {
            static bool is_type(const ValueType &v) { return v.is_int64(); }

            static int64_t get(const ValueType &v) { return v.get_int64(); }

            static ValueType &set(ValueType &v, int64_t data) { return v.set_int64(data); }

            static ValueType &set(ValueType &v, int64_t data, typename ValueType::AllocatorType &) {
                return v.set_int64(data);
            }
        };

        template<typename ValueType>
        struct TypeHelper<ValueType, uint64_t> {
            static bool is_type(const ValueType &v) { return v.is_uint64(); }

            static uint64_t get(const ValueType &v) { return v.get_uint64(); }

            static ValueType &set(ValueType &v, uint64_t data) { return v.set_uint64(data); }

            static ValueType &
            set(ValueType &v, uint64_t data, typename ValueType::AllocatorType &) { return v.set_uint64(data); }
        };

        template<typename ValueType>
        struct TypeHelper<ValueType, double> {
            static bool is_type(const ValueType &v) { return v.is_double(); }

            static double get(const ValueType &v) { return v.get_double(); }

            static ValueType &set(ValueType &v, double data) { return v.set_double(data); }

            static ValueType &set(ValueType &v, double data, typename ValueType::AllocatorType &) {
                return v.set_double(data);
            }
        };

        template<typename ValueType>
        struct TypeHelper<ValueType, float> {
            static bool is_type(const ValueType &v) { return v.is_float(); }

            static float get(const ValueType &v) { return v.get_float(); }

            static ValueType &set(ValueType &v, float data) { return v.set_float(data); }

            static ValueType &set(ValueType &v, float data, typename ValueType::AllocatorType &) {
                return v.set_float(data);
            }
        };

        template<typename ValueType>
        struct TypeHelper<ValueType, const typename ValueType::char_type *> {
            typedef const typename ValueType::char_type *StringType;

            static bool is_type(const ValueType &v) { return v.is_string(); }

            static StringType get(const ValueType &v) { return v.get_string(); }

            static ValueType &set(ValueType &v, const StringType data) {
                return v.set_string(typename ValueType::StringRefType(data));
            }

            static ValueType &set(ValueType &v, const StringType data, typename ValueType::AllocatorType &a) {
                return v.set_string(data, a);
            }
        };

        template<typename ValueType>
        struct TypeHelper<ValueType, std::basic_string<typename ValueType::char_type> > {
            typedef std::basic_string<typename ValueType::char_type> StringType;

            static bool is_type(const ValueType &v) { return v.is_string(); }

            static StringType get(const ValueType &v) { return StringType(v.get_string(), v.get_string_length()); }

            static ValueType &set(ValueType &v, const StringType &data, typename ValueType::AllocatorType &a) {
                return v.set_string(data, a);
            }
        };


        template<typename ValueType>
        struct TypeHelper<ValueType, typename ValueType::Array> {
            typedef typename ValueType::Array ArrayType;

            static bool is_type(const ValueType &v) { return v.is_array(); }

            static ArrayType get(ValueType &v) { return v.get_array(); }

            static ValueType &set(ValueType &v, ArrayType data) { return v = data; }

            static ValueType &
            set(ValueType &v, ArrayType data, typename ValueType::AllocatorType &) { return v = data; }
        };

        template<typename ValueType>
        struct TypeHelper<ValueType, typename ValueType::ConstArray> {
            typedef typename ValueType::ConstArray ArrayType;

            static bool is_type(const ValueType &v) { return v.is_array(); }

            static ArrayType get(const ValueType &v) { return v.get_array(); }
        };

        template<typename ValueType>
        struct TypeHelper<ValueType, typename ValueType::Object> {
            typedef typename ValueType::Object ObjectType;

            static bool is_type(const ValueType &v) { return v.is_object(); }

            static ObjectType get(ValueType &v) { return v.get_object(); }

            static ValueType &set(ValueType &v, ObjectType data) { return v = data; }

            static ValueType &
            set(ValueType &v, ObjectType data, typename ValueType::AllocatorType &) { return v = data; }
        };

        template<typename ValueType>
        struct TypeHelper<ValueType, typename ValueType::ConstObject> {
            typedef typename ValueType::ConstObject ObjectType;

            static bool is_type(const ValueType &v) { return v.is_object(); }

            static ObjectType get(const ValueType &v) { return v.get_object(); }
        };

    } // namespace internal

    // Forward declarations
    template<bool, typename>
    class GenericArray;

    template<bool, typename>
    class GenericObject;

    ///////////////////////////////////////////////////////////////////////////////
    // GenericValue

    //! Represents a JSON value. Use Value for UTF8 encoding and default allocator.
    /*!
        A JSON value can be one of 7 types. This class is a variant type supporting
        these types.

        Use the Value if UTF8 and default allocator

        \tparam Encoding    Encoding of the value. (Even non-string values need to have the same encoding in a document)
        \tparam Allocator   Allocator type for allocating memory of object, array and string.
    */
    template<typename Encoding, typename Allocator = MERAK_JSON_DEFAULT_ALLOCATOR >
    class GenericValue {
    public:
        //! Name-value pair in an object.
        typedef GenericMember<Encoding, Allocator> Member;
        typedef Encoding EncodingType;                  //!< Encoding type from template parameter.
        typedef Allocator AllocatorType;                //!< Allocator type from template parameter.
        typedef typename Encoding::char_type char_type;               //!< Character type derived from Encoding.
        typedef GenericStringRef<char_type> StringRefType;     //!< Reference to a constant string
        typedef typename GenericMemberIterator<false, Encoding, Allocator>::Iterator MemberIterator;  //!< Member iterator for iterating in object.
        typedef typename GenericMemberIterator<true, Encoding, Allocator>::Iterator ConstMemberIterator;  //!< Constant member iterator for iterating in object.
        typedef GenericValue *ValueIterator;            //!< Value iterator for iterating in array.
        typedef const GenericValue *ConstValueIterator; //!< Constant value iterator for iterating in array.
        typedef GenericValue<Encoding, Allocator> ValueType;    //!< Value type of itself.
        typedef GenericArray<false, ValueType> Array;
        typedef GenericArray<true, ValueType> ConstArray;
        typedef GenericObject<false, ValueType> Object;
        typedef GenericObject<true, ValueType> ConstObject;

        //!@name Constructors and destructor.
        //@{

        //! Default constructor creates a null value.
        GenericValue() MERAK_JSON_NOEXCEPT: data_() { data_.f.flags = kNullFlag; }


        //! Move constructor in C++11
        GenericValue(GenericValue &&rhs) MERAK_JSON_NOEXCEPT: data_(rhs.data_) {
            rhs.data_.f.flags = kNullFlag; // give up contents
        }

        //! Moving from a GenericDocument is not permitted.
        template<typename StackAllocator>
        GenericValue(GenericDocument<Encoding, Allocator, StackAllocator> &&rhs) = delete;

        //! Move assignment from a GenericDocument is not permitted.
        template<typename StackAllocator>
        GenericValue &operator=(GenericDocument<Encoding, Allocator, StackAllocator> &&rhs) = delete;

        //! Copy constructor is not permitted.
        GenericValue(const GenericValue &rhs) = delete;

    public:

        //! Constructor with JSON value type.
        /*! This creates a Value of specified type with default content.
            \param type Type of the value.
            \note Default content for number is zero.
        */
        explicit GenericValue(Type type) MERAK_JSON_NOEXCEPT: data_() {
            static const uint16_t defaultFlags[] = {
                    kNullFlag, kFalseFlag, kTrueFlag, kObjectFlag, kArrayFlag, kShortStringFlag,
                    kNumberAnyFlag
            };
            MERAK_JSON_NOEXCEPT_ASSERT(type >= kNullType && type <= kNumberType);
            data_.f.flags = defaultFlags[type];

            // Use ShortString to store empty string.
            if (type == kStringType)
                data_.ss.set_length(0);
        }

        //! Explicit copy constructor (with allocator)
        /*! Creates a copy of a Value by using the given Allocator
            \tparam SourceAllocator allocator of \c rhs
            \param rhs Value to copy from (read-only)
            \param allocator Allocator for allocating copied elements and buffers. Commonly use GenericDocument::get_allocator().
            \param copyConstStrings Force copying of constant strings (e.g. referencing an in-situ buffer)
            \see copy_from()
        */
        template<typename SourceAllocator>
        GenericValue(const GenericValue<Encoding, SourceAllocator> &rhs, Allocator &allocator,
                     bool copyConstStrings = false) {
            switch (rhs.get_type()) {
                case kObjectType:
                    do_copy_members(rhs, allocator, copyConstStrings);
                    break;
                case kArrayType: {
                    SizeType count = rhs.data_.a.size;
                    GenericValue *le = reinterpret_cast<GenericValue *>(allocator.Malloc(count * sizeof(GenericValue)));
                    const GenericValue<Encoding, SourceAllocator> *re = rhs.get_elements_pointer();
                    for (SizeType i = 0; i < count; i++)
                        new(&le[i]) GenericValue(re[i], allocator, copyConstStrings);
                    data_.f.flags = kArrayFlag;
                    data_.a.size = data_.a.capacity = count;
                    set_elements_pointer(le);
                }
                    break;
                case kStringType:
                    if (rhs.data_.f.flags == kConstStringFlag && !copyConstStrings) {
                        data_.f.flags = rhs.data_.f.flags;
                        data_ = *reinterpret_cast<const Data *>(&rhs.data_);
                    } else
                        set_string_raw(StringRef(rhs.get_string(), rhs.get_string_length()), allocator);
                    break;
                default:
                    data_.f.flags = rhs.data_.f.flags;
                    data_ = *reinterpret_cast<const Data *>(&rhs.data_);
                    break;
            }
        }

        //! Constructor for boolean value.
        /*! \param b Boolean value
            \note This constructor is limited to \em real boolean values and rejects
                implicitly converted types like arbitrary pointers.  Use an explicit cast
                to \c bool, if you want to construct a boolean JSON value in such cases.
         */
#ifndef MERAK_JSON_DOXYGEN_RUNNING // hide SFINAE from Doxygen

        template<typename T>
        explicit GenericValue(T b, MERAK_JSON_ENABLEIF((internal::IsSame<bool, T>))) MERAK_JSON_NOEXCEPT  // See #472
#else
        explicit GenericValue(bool b) MERAK_JSON_NOEXCEPT
#endif
                : data_() {
            // safe-guard against failing SFINAE
            MERAK_JSON_STATIC_ASSERT((internal::IsSame<bool, T>::Value));
            data_.f.flags = b ? kTrueFlag : kFalseFlag;
        }

        //! Constructor for int value.
        explicit GenericValue(int i) MERAK_JSON_NOEXCEPT: data_() {
            data_.n.i64 = i;
            data_.f.flags = (i >= 0) ? (kNumberIntFlag | kUintFlag | kUint64Flag) : kNumberIntFlag;
        }

        //! Constructor for unsigned value.
        explicit GenericValue(unsigned u) MERAK_JSON_NOEXCEPT: data_() {
            data_.n.u64 = u;
            data_.f.flags = (u & 0x80000000) ? kNumberUintFlag : (kNumberUintFlag | kIntFlag | kInt64Flag);
        }

        //! Constructor for int64_t value.
        explicit GenericValue(int64_t i64) MERAK_JSON_NOEXCEPT: data_() {
            data_.n.i64 = i64;
            data_.f.flags = kNumberInt64Flag;
            if (i64 >= 0) {
                data_.f.flags |= kNumberUint64Flag;
                if (!(static_cast<uint64_t>(i64) & MERAK_JSON_UINT64_C2(0xFFFFFFFF, 0x00000000)))
                    data_.f.flags |= kUintFlag;
                if (!(static_cast<uint64_t>(i64) & MERAK_JSON_UINT64_C2(0xFFFFFFFF, 0x80000000)))
                    data_.f.flags |= kIntFlag;
            } else if (i64 >= static_cast<int64_t>(MERAK_JSON_UINT64_C2(0xFFFFFFFF, 0x80000000)))
                data_.f.flags |= kIntFlag;
        }

        //! Constructor for uint64_t value.
        explicit GenericValue(uint64_t u64) MERAK_JSON_NOEXCEPT: data_() {
            data_.n.u64 = u64;
            data_.f.flags = kNumberUint64Flag;
            if (!(u64 & MERAK_JSON_UINT64_C2(0x80000000, 0x00000000)))
                data_.f.flags |= kInt64Flag;
            if (!(u64 & MERAK_JSON_UINT64_C2(0xFFFFFFFF, 0x00000000)))
                data_.f.flags |= kUintFlag;
            if (!(u64 & MERAK_JSON_UINT64_C2(0xFFFFFFFF, 0x80000000)))
                data_.f.flags |= kIntFlag;
        }

        //! Constructor for double value.
        explicit GenericValue(double d) MERAK_JSON_NOEXCEPT: data_() {
            data_.n.d = d;
            data_.f.flags = kNumberDoubleFlag;
        }

        //! Constructor for float value.
        explicit GenericValue(float f) MERAK_JSON_NOEXCEPT: data_() {
            data_.n.d = static_cast<double>(f);
            data_.f.flags = kNumberDoubleFlag;
        }

        //! Constructor for constant string (i.e. do not make a copy of string)
        GenericValue(const char_type *s, SizeType length) MERAK_JSON_NOEXCEPT: data_() {
            set_string_raw(StringRef(s, length));
        }

        //! Constructor for constant string (i.e. do not make a copy of string)
        explicit GenericValue(StringRefType s) MERAK_JSON_NOEXCEPT: data_() { set_string_raw(s); }

        //! Constructor for copy-string (i.e. do make a copy of string)
        GenericValue(const char_type *s, SizeType length, Allocator &allocator) : data_() {
            set_string_raw(StringRef(s, length), allocator);
        }

        //! Constructor for copy-string (i.e. do make a copy of string)
        GenericValue(const char_type *s, Allocator &allocator) : data_() { set_string_raw(StringRef(s), allocator); }

        //! Constructor for copy-string from a string object (i.e. do make a copy of string)
        /*! \note Requires the definition of the preprocessor symbol.
         */
        GenericValue(const std::basic_string<char_type> &s, Allocator &allocator) : data_() {
            set_string_raw(StringRef(s), allocator);
        }


        //! Constructor for Array.
        /*!
            \param a An array obtained by \c get_array().
            \note \c Array is always pass-by-value.
            \note the source array is moved into this value and the sourec array becomes empty.
        */
        GenericValue(Array a) MERAK_JSON_NOEXCEPT: data_(a.value_.data_) {
            a.value_.data_ = Data();
            a.value_.data_.f.flags = kArrayFlag;
        }

        //! Constructor for Object.
        /*!
            \param o An object obtained by \c get_object().
            \note \c Object is always pass-by-value.
            \note the source object is moved into this value and the sourec object becomes empty.
        */
        GenericValue(Object o) MERAK_JSON_NOEXCEPT: data_(o.value_.data_) {
            o.value_.data_ = Data();
            o.value_.data_.f.flags = kObjectFlag;
        }

        //! Destructor.
        /*! Need to destruct elements of array, members of object, or copy-string.
        */
        ~GenericValue() {
            // With MERAK_JSON_USE_MEMBERSMAP, the maps need to be destroyed to release
            // their Allocator if it's refcounted (e.g. MemoryPoolAllocator).
            if (Allocator::kNeedFree || (MERAK_JSON_USE_MEMBERSMAP + 0 &&
                                         internal::IsRefCounted<Allocator>::Value)) {
                switch (data_.f.flags) {
                    case kArrayFlag: {
                        GenericValue *e = get_elements_pointer();
                        for (GenericValue *v = e; v != e + data_.a.size; ++v)
                            v->~GenericValue();
                        if (Allocator::kNeedFree) { // Shortcut by Allocator's trait
                            Allocator::Free(e);
                        }
                    }
                        break;

                    case kObjectFlag:
                        do_free_members();
                        break;

                    case kCopyStringFlag:
                        if (Allocator::kNeedFree) { // Shortcut by Allocator's trait
                            Allocator::Free(const_cast<char_type *>(get_string_pointer()));
                        }
                        break;

                    default:
                        break;  // Do nothing for other types.
                }
            }
        }

        //@}

        //!@name Assignment operators
        //@{

        //! Assignment with move semantics.
        /*! \param rhs Source of the assignment. It will become a null value after assignment.
        */
        GenericValue &operator=(GenericValue &rhs) MERAK_JSON_NOEXCEPT {
            if (MERAK_JSON_LIKELY(this != &rhs)) {
                // Can't destroy "this" before assigning "rhs", otherwise "rhs"
                // could be used after free if it's an sub-Value of "this",
                // hence the temporary danse.
                GenericValue temp;
                temp.raw_assign(rhs);
                this->~GenericValue();
                raw_assign(temp);
            }
            return *this;
        }


        //! Move assignment in C++11
        GenericValue &operator=(GenericValue &&rhs) MERAK_JSON_NOEXCEPT {
             *this = rhs.Move();
            return *this;
        }


        //! Assignment of constant string reference (no copy)
        /*! \param str Constant string reference to be assigned
            \note This overload is needed to avoid clashes with the generic primitive type assignment overload below.
            \see GenericStringRef, operator=(T)
        */
        GenericValue &operator=(StringRefType str) MERAK_JSON_NOEXCEPT {
            GenericValue s(str);
            *this = s;
            return *this;
        }

        //! Assignment with primitive types.
        /*! \tparam T Either \ref Type, \c int, \c unsigned, \c int64_t, \c uint64_t
            \param value The value to be assigned.

            \note The source type \c T explicitly disallows all pointer types,
                especially (\c const) \ref char_type*.  This helps avoiding implicitly
                referencing character strings with insufficient lifetime, use
                \ref set_string(const char_type*, Allocator&) (for copying) or
                \ref StringRef() (to explicitly mark the pointer as constant) instead.
                All other pointer types would implicitly convert to \c bool,
                use \ref SetBool() instead.
        */
        template<typename T>
        MERAK_JSON_DISABLEIF_RETURN((internal::IsPointer<T>), (GenericValue & ))
        operator=(T value) {
            GenericValue v(value);
            return *this = v;
        }

        //! Deep-copy assignment from Value
        /*! Assigns a \b copy of the Value to the current Value object
            \tparam SourceAllocator Allocator type of \c rhs
            \param rhs Value to copy from (read-only)
            \param allocator Allocator to use for copying
            \param copyConstStrings Force copying of constant strings (e.g. referencing an in-situ buffer)
         */
        template<typename SourceAllocator>
        GenericValue &copy_from(const GenericValue<Encoding, SourceAllocator> &rhs, Allocator &allocator,
                                bool copyConstStrings = false) {
            MERAK_JSON_ASSERT(static_cast<void *>(this) != static_cast<void const *>(&rhs));
            this->~GenericValue();
            new(this) GenericValue(rhs, allocator, copyConstStrings);
            return *this;
        }

        //! Exchange the contents of this value with those of other.
        /*!
            \param other Another value.
            \note Constant complexity.
        */
        GenericValue &swap(GenericValue &other) MERAK_JSON_NOEXCEPT {
            GenericValue temp;
            temp.raw_assign(*this);
            raw_assign(other);
            other.raw_assign(temp);
            return *this;
        }

        //! free-standing swap function helper
        /*!
            Helper function to enable support for common swap implementation pattern based on \c std::swap:
            \code
            void swap(MyClass& a, MyClass& b) {
                using std::swap;
                swap(a.value, b.value);
                // ...
            }
            \endcode
            \see swap()
         */
        friend inline void swap(GenericValue &a, GenericValue &b) MERAK_JSON_NOEXCEPT { a.swap(b); }

        //! Prepare Value for move semantics
        /*! \return *this */
        GenericValue &Move() MERAK_JSON_NOEXCEPT { return *this; }
        //@}

        //!@name Equal-to and not-equal-to operators
        //@{
        //! Equal-to operator
        /*!
            \note If an object contains duplicated named member, comparing equality with any object is always \c false.
            \note Complexity is quadratic in Object's member number and linear for the rest (number of all values in the subtree and total lengths of all strings).
        */
        template<typename SourceAllocator>
        bool operator==(const GenericValue<Encoding, SourceAllocator> &rhs) const {
            typedef GenericValue<Encoding, SourceAllocator> RhsType;
            if (get_type() != rhs.get_type())
                return false;

            switch (get_type()) {
                case kObjectType: // Warning: O(n^2) inner-loop
                    if (data_.o.size != rhs.data_.o.size)
                        return false;
                    for (ConstMemberIterator lhsMemberItr = member_begin();
                         lhsMemberItr != member_end(); ++lhsMemberItr) {
                        typename RhsType::ConstMemberIterator rhsMemberItr = rhs.find_member(lhsMemberItr->name);
                        if (rhsMemberItr == rhs.member_end() || (!(lhsMemberItr->value == rhsMemberItr->value)))
                            return false;
                    }
                    return true;

                case kArrayType:
                    if (data_.a.size != rhs.data_.a.size)
                        return false;
                    for (SizeType i = 0; i < data_.a.size; i++)
                        if (!((*this)[i] == rhs[i]))
                            return false;
                    return true;

                case kStringType:
                    return string_equal(rhs);

                case kNumberType:
                    if (is_double() || rhs.is_double()) {
                        double a = get_double();     // May convert from integer to double.
                        double b = rhs.get_double(); // Ditto
                        return a >= b && a <= b;    // Prevent -Wfloat-equal
                    } else
                        return data_.n.u64 == rhs.data_.n.u64;

                default:
                    return true;
            }
        }

        //! Equal-to operator with const C-string pointer
        bool operator==(const char_type *rhs) const { return *this == GenericValue(StringRef(rhs)); }

        //! Equal-to operator with string object
        /*! \note Requires the definition of the preprocessor symbol.
         */
        bool operator==(const std::basic_string<char_type> &rhs) const { return *this == GenericValue(StringRef(rhs)); }


        //! Equal-to operator with primitive types
        /*! \tparam T Either \ref Type, \c int, \c unsigned, \c int64_t, \c uint64_t, \c double, \c true, \c false
        */
        template<typename T>
        MERAK_JSON_DISABLEIF_RETURN((internal::OrExpr<internal::IsPointer<T>, internal::IsGenericValue<T> >), (bool))
        operator==(const T &rhs) const { return *this == GenericValue(rhs); }

#ifndef __cpp_impl_three_way_comparison
        //! Not-equal-to operator
        /*! \return !(*this == rhs)
         */
        template<typename SourceAllocator>
        bool operator!=(const GenericValue<Encoding, SourceAllocator> &rhs) const { return !(*this == rhs); }

        //! Not-equal-to operator with const C-string pointer
        bool operator!=(const char_type *rhs) const { return !(*this == rhs); }

        //! Not-equal-to operator with arbitrary types
        /*! \return !(*this == rhs)
         */
        template<typename T>
        MERAK_JSON_DISABLEIF_RETURN((internal::IsGenericValue<T>), (bool)) operator!=(const T &rhs) const {
            return !(*this == rhs);
        }

        //! Equal-to operator with arbitrary types (symmetric version)
        /*! \return (rhs == lhs)
         */
        template<typename T>
        friend MERAK_JSON_DISABLEIF_RETURN((internal::IsGenericValue<T>), (bool))
        operator==(const T &lhs, const GenericValue &rhs) { return rhs == lhs; }

        //! Not-Equal-to operator with arbitrary types (symmetric version)
        /*! \return !(rhs == lhs)
         */
        template<typename T>
        friend MERAK_JSON_DISABLEIF_RETURN((internal::IsGenericValue<T>), (bool))
        operator!=(const T &lhs, const GenericValue &rhs) { return !(rhs == lhs); }
        //@}
#endif

        //!@name Type
        //@{

        [[nodiscard]] constexpr Type get_type() const { return static_cast<Type>(data_.f.flags & kTypeMask); }

        [[nodiscard]] constexpr bool is_null() const { return data_.f.flags == kNullFlag; }

        [[nodiscard]] constexpr bool is_false() const { return data_.f.flags == kFalseFlag; }

        [[nodiscard]] constexpr bool is_true() const { return data_.f.flags == kTrueFlag; }

        [[nodiscard]] constexpr bool is_bool() const { return (data_.f.flags & kBoolFlag) != 0; }

        [[nodiscard]] constexpr bool is_object() const { return data_.f.flags == kObjectFlag; }

        [[nodiscard]] constexpr bool is_array() const { return data_.f.flags == kArrayFlag; }

        [[nodiscard]] constexpr bool is_number() const { return (data_.f.flags & kNumberFlag) != 0; }

        [[nodiscard]] constexpr bool is_int32() const { return (data_.f.flags & kIntFlag) != 0; }

        [[nodiscard]] constexpr bool is_uint32() const { return (data_.f.flags & kUintFlag) != 0; }

        [[nodiscard]] constexpr bool is_int64() const { return (data_.f.flags & kInt64Flag) != 0; }

        [[nodiscard]] constexpr bool is_uint64() const { return (data_.f.flags & kUint64Flag) != 0; }

        [[nodiscard]] constexpr bool is_double() const { return (data_.f.flags & kDoubleFlag) != 0; }

        [[nodiscard]] constexpr bool is_string() const { return (data_.f.flags & kStringFlag) != 0; }

        // Checks whether a number can be losslessly converted to a double.
        [[nodiscard]] bool is_lossless_double() const {
            if (!is_number()) return false;
            if (is_uint64()) {
                uint64_t u = get_uint64();
                volatile auto d = static_cast<double>(u);
                return (d >= 0.0)
                       && (d < static_cast<double>((std::numeric_limits<uint64_t>::max)()))
                       && (u == static_cast<uint64_t>(d));
            }
            if (is_int64()) {
                int64_t i = get_int64();
                volatile auto d = static_cast<double>(i);
                return (d >= static_cast<double>((std::numeric_limits<int64_t>::min)()))
                       && (d < static_cast<double>((std::numeric_limits<int64_t>::max)()))
                       && (i == static_cast<int64_t>(d));
            }
            return true; // double, int, uint are always lossless
        }

        // Checks whether a number is a float (possible lossy).
        [[nodiscard]] bool is_float() const {
            if ((data_.f.flags & kDoubleFlag) == 0)
                return false;
            double d = get_double();
            return d >= -3.4028234e38 && d <= 3.4028234e38;
        }

        // Checks whether a number can be losslessly converted to a float.
        [[nodiscard]] bool is_lossless_float() const {
            if (!is_number()) return false;
            double a = get_double();
            if (a < static_cast<double>(-(std::numeric_limits<float>::max)())
                || a > static_cast<double>((std::numeric_limits<float>::max)()))
                return false;
            auto b = static_cast<double>(static_cast<float>(a));
            return a >= b && a <= b;    // Prevent -Wfloat-equal
        }

        //@}

        //!@name Null
        //@{

        GenericValue &SetNull() {
            this->~GenericValue();
            new(this) GenericValue();
            return *this;
        }

        //@}

        //!@name Bool
        //@{

        bool get_bool() const {
            MERAK_JSON_ASSERT(is_bool());
            return data_.f.flags == kTrueFlag;
        }
        //!< Set boolean value
        /*! \post is_bool() == true */
        GenericValue &SetBool(bool b) {
            this->~GenericValue();
            new(this) GenericValue(b);
            return *this;
        }

        //@}

        //!@name Object
        //@{

        //! Set this value as an empty object.
        /*! \post is_object() == true */
        GenericValue &SetObject() {
            this->~GenericValue();
            new(this) GenericValue(kObjectType);
            return *this;
        }

        //! Get the number of members in the object.
        SizeType member_count() const {
            MERAK_JSON_ASSERT(is_object());
            return data_.o.size;
        }

        //! Get the capacity of object.
        SizeType member_capacity() const {
            MERAK_JSON_ASSERT(is_object());
            return data_.o.capacity;
        }

        //! Check whether the object is empty.
        bool object_empty() const {
            MERAK_JSON_ASSERT(is_object());
            return data_.o.size == 0;
        }

        //! Get a value from an object associated with the name.
        /*! \pre is_object() == true
            \tparam T Either \c char_type or \c const \c char_type (template used for disambiguation with \ref operator[](SizeType))
            \note In version 0.1x, if the member is not found, this function returns a null value. This makes issue 7.
            Since 0.2, if the name is not correct, it will assert.
            If user is unsure whether a member exists, user should use has_member() first.
            A better approach is to use find_member().
            \note Linear time complexity.
        */
        template<typename T>
        MERAK_JSON_DISABLEIF_RETURN((internal::NotExpr<internal::IsSame<typename internal::RemoveConst<T>::Type, char_type> >),
                                    (GenericValue & )) operator[](T *name) {
            GenericValue n(StringRef(name));
            return (*this)[n];
        }

        template<typename T>
        MERAK_JSON_DISABLEIF_RETURN((internal::NotExpr<internal::IsSame<typename internal::RemoveConst<T>::Type, char_type> >),
                                    (const GenericValue&))
        operator[](T *name) const { return const_cast<GenericValue &>(*this)[name]; }

        //! Get a value from an object associated with the name.
        /*! \pre is_object() == true
            \tparam SourceAllocator Allocator of the \c name value

            \note Compared to \ref operator[](T*), this version is faster because it does not need a StrLen().
            And it can also handle strings with embedded null characters.

            \note Linear time complexity.
        */
        template<typename SourceAllocator>
        GenericValue &operator[](const GenericValue<Encoding, SourceAllocator> &name) {
            MemberIterator member = find_member(name);
            if (member != member_end())
                return member->value;
            else {
                MERAK_JSON_ASSERT(false);    // see above note
                // Use thread-local storage to prevent races between threads.
                // Use static buffer and placement-new to prevent destruction, with
                // alignas() to ensure proper alignment.
                alignas(GenericValue) thread_local static char buffer[sizeof(GenericValue)];
                return *new(buffer) GenericValue();
            }
        }

        template<typename SourceAllocator>
        const GenericValue &operator[](
                const GenericValue<Encoding, SourceAllocator> &name) const { return const_cast<GenericValue &>(*this)[name]; }

        //! Get a value from an object associated with name (string object).
        GenericValue &operator[](const std::basic_string<char_type> &name) { return (*this)[GenericValue(StringRef(name))]; }

        const GenericValue &operator[](const std::basic_string<char_type> &name) const {
            return (*this)[GenericValue(StringRef(name))];
        }

        //! Const member iterator
        /*! \pre is_object() == true */
        ConstMemberIterator member_begin() const {
            MERAK_JSON_ASSERT(is_object());
            return ConstMemberIterator(get_members_pointer());
        }
        //! Const \em past-the-end member iterator
        /*! \pre is_object() == true */
        ConstMemberIterator member_end() const {
            MERAK_JSON_ASSERT(is_object());
            return ConstMemberIterator(get_members_pointer() + data_.o.size);
        }
        //! Member iterator
        /*! \pre is_object() == true */
        MemberIterator member_begin() {
            MERAK_JSON_ASSERT(is_object());
            return MemberIterator(get_members_pointer());
        }
        //! \em Past-the-end member iterator
        /*! \pre is_object() == true */
        MemberIterator member_end() {
            MERAK_JSON_ASSERT(is_object());
            return MemberIterator(get_members_pointer() + data_.o.size);
        }

        //! Request the object to have enough capacity to store members.
        /*! \param newCapacity  The capacity that the object at least need to have.
            \param allocator    Allocator for reallocating memory. It must be the same one as used before. Commonly use GenericDocument::get_allocator().
            \return The value itself for fluent API.
            \note Linear time complexity.
        */
        GenericValue &member_reserve(SizeType newCapacity, Allocator &allocator) {
            MERAK_JSON_ASSERT(is_object());
            do_reserve_members(newCapacity, allocator);
            return *this;
        }

        //! Check whether a member exists in the object.
        /*!
            \param name Member name to be searched.
            \pre is_object() == true
            \return Whether a member with that name exists.
            \note It is better to use find_member() directly if you need the obtain the value as well.
            \note Linear time complexity.
        */
        bool has_member(const char_type *name) const { return find_member(name) != member_end(); }

        //! Check whether a member exists in the object with string object.
        /*!
            \param name Member name to be searched.
            \pre is_object() == true
            \return Whether a member with that name exists.
            \note It is better to use find_member() directly if you need the obtain the value as well.
            \note Linear time complexity.
        */
        bool has_member(const std::basic_string<char_type> &name) const { return find_member(name) != member_end(); }

        //! Check whether a member exists in the object with GenericValue name.
        /*!
            This version is faster because it does not need a StrLen(). It can also handle string with null character.
            \param name Member name to be searched.
            \pre is_object() == true
            \return Whether a member with that name exists.
            \note It is better to use find_member() directly if you need the obtain the value as well.
            \note Linear time complexity.
        */
        template<typename SourceAllocator>
        bool has_member(const GenericValue<Encoding, SourceAllocator> &name) const {
            return find_member(name) != member_end();
        }

        //! Find member by name.
        /*!
            \param name Member name to be searched.
            \pre is_object() == true
            \return Iterator to member, if it exists.
                Otherwise returns \ref member_end().

            \note Earlier versions of Rapidjson returned a \c NULL pointer, in case
                the requested member doesn't exist. For consistency with e.g.
                \c std::map, this has been changed to member_end() now.
            \note Linear time complexity.
        */
        MemberIterator find_member(const char_type *name) {
            GenericValue n(StringRef(name));
            return find_member(n);
        }

        ConstMemberIterator find_member(const char_type *name) const {
            return const_cast<GenericValue &>(*this).find_member(name);
        }

        //! Find member by name.
        /*!
            This version is faster because it does not need a StrLen(). It can also handle string with null character.
            \param name Member name to be searched.
            \pre is_object() == true
            \return Iterator to member, if it exists.
                Otherwise returns \ref member_end().

            \note Earlier versions of Rapidjson returned a \c NULL pointer, in case
                the requested member doesn't exist. For consistency with e.g.
                \c std::map, this has been changed to member_end() now.
            \note Linear time complexity.
        */
        template<typename SourceAllocator>
        MemberIterator find_member(const GenericValue<Encoding, SourceAllocator> &name) {
            MERAK_JSON_ASSERT(is_object());
            MERAK_JSON_ASSERT(name.is_string());
            return do_find_member(name);
        }

        template<typename SourceAllocator>
        ConstMemberIterator find_member(const GenericValue<Encoding, SourceAllocator> &name) const {
            return const_cast<GenericValue &>(*this).find_member(name);
        }

        //! Find member by string object name.
        /*!
            \param name Member name to be searched.
            \pre is_object() == true
            \return Iterator to member, if it exists.
                Otherwise returns \ref member_end().
        */
        MemberIterator find_member(const std::basic_string<char_type> &name) {
            return find_member(GenericValue(StringRef(name)));
        }

        ConstMemberIterator find_member(const std::basic_string<char_type> &name) const {
            return find_member(GenericValue(StringRef(name)));
        }

        //! Add a member (name-value pair) to the object.
        /*! \param name A string value as name of member.
            \param value Value of any type.
            \param allocator    Allocator for reallocating memory. It must be the same one as used before. Commonly use GenericDocument::get_allocator().
            \return The value itself for fluent API.
            \note The ownership of \c name and \c value will be transferred to this object on success.
            \pre  is_object() && name.is_string()
            \post name.is_null() && value.is_null()
            \note Amortized Constant time complexity.
        */
        GenericValue &add_member(GenericValue &name, GenericValue &value, Allocator &allocator) {
            MERAK_JSON_ASSERT(is_object());
            MERAK_JSON_ASSERT(name.is_string());
            do_add_member(name, value, allocator);
            return *this;
        }

        //! Add a constant string value as member (name-value pair) to the object.
        /*! \param name A string value as name of member.
            \param value constant string reference as value of member.
            \param allocator    Allocator for reallocating memory. It must be the same one as used before. Commonly use GenericDocument::get_allocator().
            \return The value itself for fluent API.
            \pre  is_object()
            \note This overload is needed to avoid clashes with the generic primitive type add_member(GenericValue&,T,Allocator&) overload below.
            \note Amortized Constant time complexity.
        */
        GenericValue &add_member(GenericValue &name, StringRefType value, Allocator &allocator) {
            GenericValue v(value);
            return add_member(name, v, allocator);
        }

        //! Add a string object as member (name-value pair) to the object.
        /*! \param name A string value as name of member.
            \param value constant string reference as value of member.
            \param allocator    Allocator for reallocating memory. It must be the same one as used before. Commonly use GenericDocument::get_allocator().
            \return The value itself for fluent API.
            \pre  is_object()
            \note This overload is needed to avoid clashes with the generic primitive type add_member(GenericValue&,T,Allocator&) overload below.
            \note Amortized Constant time complexity.
        */
        GenericValue &add_member(GenericValue &name, std::basic_string<char_type> &value, Allocator &allocator) {
            GenericValue v(value, allocator);
            return add_member(name, v, allocator);
        }

        //! Add any primitive value as member (name-value pair) to the object.
        /*! \tparam T Either \ref Type, \c int, \c unsigned, \c int64_t, \c uint64_t
            \param name A string value as name of member.
            \param value Value of primitive type \c T as value of member
            \param allocator Allocator for reallocating memory. Commonly use GenericDocument::get_allocator().
            \return The value itself for fluent API.
            \pre  is_object()

            \note The source type \c T explicitly disallows all pointer types,
                especially (\c const) \ref char_type*.  This helps avoiding implicitly
                referencing character strings with insufficient lifetime, use
                \ref add_member(StringRefType, GenericValue&, Allocator&) or \ref
                add_member(StringRefType, StringRefType, Allocator&).
                All other pointer types would implicitly convert to \c bool,
                use an explicit cast instead, if needed.
            \note Amortized Constant time complexity.
        */
        template<typename T>
        MERAK_JSON_DISABLEIF_RETURN((internal::OrExpr<internal::IsPointer<T>, internal::IsGenericValue<T> >),
                                    (GenericValue & ))
        add_member(GenericValue &name, T value, Allocator &allocator) {
            GenericValue v(value);
            return add_member(name, v, allocator);
        }


        GenericValue &add_member(GenericValue &&name, GenericValue &&value, Allocator &allocator) {
            return add_member(name, value, allocator);
        }

        GenericValue &add_member(GenericValue &&name, GenericValue &value, Allocator &allocator) {
            return add_member(name, value, allocator);
        }

        GenericValue &add_member(GenericValue &name, GenericValue &&value, Allocator &allocator) {
            return add_member(name, value, allocator);
        }

        GenericValue &add_member(StringRefType name, GenericValue &&value, Allocator &allocator) {
            GenericValue n(name);
            return add_member(n, value, allocator);
        }



        //! Add a member (name-value pair) to the object.
        /*! \param name A constant string reference as name of member.
            \param value Value of any type.
            \param allocator    Allocator for reallocating memory. It must be the same one as used before. Commonly use GenericDocument::get_allocator().
            \return The value itself for fluent API.
            \note The ownership of \c value will be transferred to this object on success.
            \pre  is_object()
            \post value.is_null()
            \note Amortized Constant time complexity.
        */
        GenericValue &add_member(StringRefType name, GenericValue &value, Allocator &allocator) {
            GenericValue n(name);
            return add_member(n, value, allocator);
        }

        //! Add a constant string value as member (name-value pair) to the object.
        /*! \param name A constant string reference as name of member.
            \param value constant string reference as value of member.
            \param allocator    Allocator for reallocating memory. It must be the same one as used before. Commonly use GenericDocument::get_allocator().
            \return The value itself for fluent API.
            \pre  is_object()
            \note This overload is needed to avoid clashes with the generic primitive type add_member(StringRefType,T,Allocator&) overload below.
            \note Amortized Constant time complexity.
        */
        GenericValue &add_member(StringRefType name, StringRefType value, Allocator &allocator) {
            GenericValue v(value);
            return add_member(name, v, allocator);
        }

        //! Add any primitive value as member (name-value pair) to the object.
        /*! \tparam T Either \ref Type, \c int, \c unsigned, \c int64_t, \c uint64_t
            \param name A constant string reference as name of member.
            \param value Value of primitive type \c T as value of member
            \param allocator Allocator for reallocating memory. Commonly use GenericDocument::get_allocator().
            \return The value itself for fluent API.
            \pre  is_object()

            \note The source type \c T explicitly disallows all pointer types,
                especially (\c const) \ref char_type*.  This helps avoiding implicitly
                referencing character strings with insufficient lifetime, use
                \ref add_member(StringRefType, GenericValue&, Allocator&) or \ref
                add_member(StringRefType, StringRefType, Allocator&).
                All other pointer types would implicitly convert to \c bool,
                use an explicit cast instead, if needed.
            \note Amortized Constant time complexity.
        */
        template<typename T>
        MERAK_JSON_DISABLEIF_RETURN((internal::OrExpr<internal::IsPointer<T>, internal::IsGenericValue<T> >),
                                    (GenericValue & ))
        add_member(StringRefType name, T value, Allocator &allocator) {
            GenericValue n(name);
            return add_member(n, value, allocator);
        }

        //! Remove all members in the object.
        /*! This function do not deallocate memory in the object, i.e. the capacity is unchanged.
            \note Linear time complexity.
        */
        void RemoveAllMembers() {
            MERAK_JSON_ASSERT(is_object());
            do_clear_members();
        }

        //! Remove a member in object by its name.
        /*! \param name Name of member to be removed.
            \return Whether the member existed.
            \note This function may reorder the object members. Use \ref
                erase_member(ConstMemberIterator) if you need to preserve the
                relative order of the remaining members.
            \note Linear time complexity.
        */
        bool remove_member(const char_type *name) {
            GenericValue n(StringRef(name));
            return remove_member(n);
        }

        bool remove_member(const std::basic_string<char_type> &name) { return remove_member(GenericValue(StringRef(name))); }


        template<typename SourceAllocator>
        bool remove_member(const GenericValue<Encoding, SourceAllocator> &name) {
            MemberIterator m = find_member(name);
            if (m != member_end()) {
                remove_member(m);
                return true;
            } else
                return false;
        }

        //! Remove a member in object by iterator.
        /*! \param m member iterator (obtained by find_member() or member_begin()).
            \return the new iterator after removal.
            \note This function may reorder the object members. Use \ref
                erase_member(ConstMemberIterator) if you need to preserve the
                relative order of the remaining members.
            \note Constant time complexity.
        */
        MemberIterator remove_member(MemberIterator m) {
            MERAK_JSON_ASSERT(is_object());
            MERAK_JSON_ASSERT(data_.o.size > 0);
            MERAK_JSON_ASSERT(get_members_pointer() != 0);
            MERAK_JSON_ASSERT(m >= member_begin() && m < member_end());
            return do_remove_member(m);
        }

        //! Remove a member from an object by iterator.
        /*! \param pos iterator to the member to remove
            \pre is_object() == true && \ref member_begin() <= \c pos < \ref member_end()
            \return Iterator following the removed element.
                If the iterator \c pos refers to the last element, the \ref member_end() iterator is returned.
            \note This function preserves the relative order of the remaining object
                members. If you do not need this, use the more efficient \ref remove_member(MemberIterator).
            \note Linear time complexity.
        */
        MemberIterator erase_member(ConstMemberIterator pos) {
            return erase_member(pos, pos + 1);
        }

        //! Remove members in the range [first, last) from an object.
        /*! \param first iterator to the first member to remove
            \param last  iterator following the last member to remove
            \pre is_object() == true && \ref member_begin() <= \c first <= \c last <= \ref member_end()
            \return Iterator following the last removed element.
            \note This function preserves the relative order of the remaining object
                members.
            \note Linear time complexity.
        */
        MemberIterator erase_member(ConstMemberIterator first, ConstMemberIterator last) {
            MERAK_JSON_ASSERT(is_object());
            MERAK_JSON_ASSERT(data_.o.size > 0);
            MERAK_JSON_ASSERT(get_members_pointer() != 0);
            MERAK_JSON_ASSERT(first >= member_begin());
            MERAK_JSON_ASSERT(first <= last);
            MERAK_JSON_ASSERT(last <= member_end());
            return do_erase_members(first, last);
        }

        //! erase a member in object by its name.
        /*! \param name Name of member to be removed.
            \return Whether the member existed.
            \note Linear time complexity.
        */
        bool erase_member(const char_type *name) {
            GenericValue n(StringRef(name));
            return erase_member(n);
        }

        bool erase_member(const std::basic_string<char_type> &name) { return erase_member(GenericValue(StringRef(name))); }


        template<typename SourceAllocator>
        bool erase_member(const GenericValue<Encoding, SourceAllocator> &name) {
            MemberIterator m = find_member(name);
            if (m != member_end()) {
                erase_member(m);
                return true;
            } else
                return false;
        }

        Object get_object() {
            MERAK_JSON_ASSERT(is_object());
            return Object(*this);
        }

        ConstObject get_object() const {
            MERAK_JSON_ASSERT(is_object());
            return ConstObject(*this);
        }

        //@}

        //!@name Array
        //@{

        //! Set this value as an empty array.
        /*! \post is_array == true */
        GenericValue &set_array() {
            this->~GenericValue();
            new(this) GenericValue(kArrayType);
            return *this;
        }

        //! Get the number of elements in array.
        [[nodiscard]] constexpr SizeType size() const {
            MERAK_JSON_ASSERT(is_array());
            return data_.a.size;
        }

        //! Get the capacity of array.
        [[nodiscard]] constexpr SizeType capacity() const {
            MERAK_JSON_ASSERT(is_array());
            return data_.a.capacity;
        }

        //! Check whether the array is empty.
        [[nodiscard]] constexpr bool empty() const {
            MERAK_JSON_ASSERT(is_array());
            return data_.a.size == 0;
        }

        //! Remove all elements in the array.
        /*! This function do not deallocate memory in the array, i.e. the capacity is unchanged.
            \note Linear time complexity.
        */
        void clear() {
            MERAK_JSON_ASSERT(is_array());
            GenericValue *e = get_elements_pointer();
            for (GenericValue *v = e; v != e + data_.a.size; ++v)
                v->~GenericValue();
            data_.a.size = 0;
        }

        //! Get an element from array by index.
        /*! \pre is_array() == true
            \param index Zero-based index of element.
            \see operator[](T*)
        */
        GenericValue &operator[](SizeType index) {
            MERAK_JSON_ASSERT(is_array());
            MERAK_JSON_ASSERT(index < data_.a.size);
            return get_elements_pointer()[index];
        }

        const GenericValue &operator[](SizeType index) const { return const_cast<GenericValue &>(*this)[index]; }

        //! Element iterator
        /*! \pre is_array() == true */
        ValueIterator begin() {
            MERAK_JSON_ASSERT(is_array());
            return get_elements_pointer();
        }
        //! \em Past-the-end element iterator
        /*! \pre is_array() == true */
        ValueIterator end() {
            MERAK_JSON_ASSERT(is_array());
            return get_elements_pointer() + data_.a.size;
        }
        //! Constant element iterator
        /*! \pre is_array() == true */
        ConstValueIterator begin() const { return const_cast<GenericValue &>(*this).begin(); }
        //! Constant \em past-the-end element iterator
        /*! \pre is_array() == true */
        ConstValueIterator end() const { return const_cast<GenericValue &>(*this).end(); }

        //! Request the array to have enough capacity to store elements.
        /*! \param newCapacity  The capacity that the array at least need to have.
            \param allocator    Allocator for reallocating memory. It must be the same one as used before. Commonly use GenericDocument::get_allocator().
            \return The value itself for fluent API.
            \note Linear time complexity.
        */
        GenericValue &reserve(SizeType newCapacity, Allocator &allocator) {
            MERAK_JSON_ASSERT(is_array());
            if (newCapacity > data_.a.capacity) {
                set_elements_pointer(reinterpret_cast<GenericValue *>(allocator.Realloc(get_elements_pointer(),
                                                                                        data_.a.capacity *
                                                                                        sizeof(GenericValue),
                                                                                        newCapacity *
                                                                                        sizeof(GenericValue))));
                data_.a.capacity = newCapacity;
            }
            return *this;
        }

        //! Append a GenericValue at the end of the array.
        /*! \param value        Value to be appended.
            \param allocator    Allocator for reallocating memory. It must be the same one as used before. Commonly use GenericDocument::get_allocator().
            \pre is_array() == true
            \post value.is_null() == true
            \return The value itself for fluent API.
            \note The ownership of \c value will be transferred to this array on success.
            \note If the number of elements to be appended is known, calls reserve() once first may be more efficient.
            \note Amortized constant time complexity.
        */
        GenericValue &push_back(GenericValue &value, Allocator &allocator) {
            MERAK_JSON_ASSERT(is_array());
            if (data_.a.size >= data_.a.capacity)
                reserve(data_.a.capacity == 0 ? kDefaultArrayCapacity : (data_.a.capacity + (data_.a.capacity + 1) / 2),
                        allocator);
            get_elements_pointer()[data_.a.size++].raw_assign(value);
            return *this;
        }


        GenericValue &push_back(GenericValue &&value, Allocator &allocator) {
            return push_back(value, allocator);
        }


        //! Append a constant string reference at the end of the array.
        /*! \param value        Constant string reference to be appended.
            \param allocator    Allocator for reallocating memory. It must be the same one used previously. Commonly use GenericDocument::get_allocator().
            \pre is_array() == true
            \return The value itself for fluent API.
            \note If the number of elements to be appended is known, calls reserve() once first may be more efficient.
            \note Amortized constant time complexity.
            \see GenericStringRef
        */
        GenericValue &push_back(StringRefType value, Allocator &allocator) {
            return (*this).template push_back<StringRefType>(value, allocator);
        }

        //! Append a primitive value at the end of the array.
        /*! \tparam T Either \ref Type, \c int, \c unsigned, \c int64_t, \c uint64_t
            \param value Value of primitive type T to be appended.
            \param allocator    Allocator for reallocating memory. It must be the same one as used before. Commonly use GenericDocument::get_allocator().
            \pre is_array() == true
            \return The value itself for fluent API.
            \note If the number of elements to be appended is known, calls reserve() once first may be more efficient.

            \note The source type \c T explicitly disallows all pointer types,
                especially (\c const) \ref char_type*.  This helps avoiding implicitly
                referencing character strings with insufficient lifetime, use
                \ref push_back(GenericValue&, Allocator&) or \ref
                push_back(StringRefType, Allocator&).
                All other pointer types would implicitly convert to \c bool,
                use an explicit cast instead, if needed.
            \note Amortized constant time complexity.
        */
        template<typename T>
        MERAK_JSON_DISABLEIF_RETURN((internal::OrExpr<internal::IsPointer<T>, internal::IsGenericValue<T> >),
                                    (GenericValue & ))
        push_back(T value, Allocator &allocator) {
            GenericValue v(value);
            return push_back(v, allocator);
        }

        //! Remove the last element in the array.
        /*!
            \note Constant time complexity.
        */
        GenericValue &pop_back() {
            MERAK_JSON_ASSERT(is_array());
            MERAK_JSON_ASSERT(!empty());
            get_elements_pointer()[--data_.a.size].~GenericValue();
            return *this;
        }

        //! Remove an element of array by iterator.
        /*!
            \param pos iterator to the element to remove
            \pre is_array() == true && \ref begin() <= \c pos < \ref end()
            \return Iterator following the removed element. If the iterator pos refers to the last element, the end() iterator is returned.
            \note Linear time complexity.
        */
        ValueIterator erase(ConstValueIterator pos) {
            return erase(pos, pos + 1);
        }

        //! Remove elements in the range [first, last) of the array.
        /*!
            \param first iterator to the first element to remove
            \param last  iterator following the last element to remove
            \pre is_array() == true && \ref begin() <= \c first <= \c last <= \ref end()
            \return Iterator following the last removed element.
            \note Linear time complexity.
        */
        ValueIterator erase(ConstValueIterator first, ConstValueIterator last) {
            MERAK_JSON_ASSERT(is_array());
            MERAK_JSON_ASSERT(data_.a.size > 0);
            MERAK_JSON_ASSERT(get_elements_pointer() != 0);
            MERAK_JSON_ASSERT(first >= begin());
            MERAK_JSON_ASSERT(first <= last);
            MERAK_JSON_ASSERT(last <= end());
            ValueIterator pos = begin() + (first - begin());
            for (ValueIterator itr = pos; itr != last; ++itr)
                itr->~GenericValue();
            std::memmove(static_cast<void *>(pos), last, static_cast<size_t>(end() - last) * sizeof(GenericValue));
            data_.a.size -= static_cast<SizeType>(last - first);
            return pos;
        }

        Array get_array() {
            MERAK_JSON_ASSERT(is_array());
            return Array(*this);
        }

        ConstArray get_array() const {
            MERAK_JSON_ASSERT(is_array());
            return ConstArray(*this);
        }

        //@}

        //!@name Number
        //@{

        [[nodiscard]] constexpr int get_int32() const {
            MERAK_JSON_ASSERT(data_.f.flags & kIntFlag);
            return data_.n.i.i;
        }

        [[nodiscard]] constexpr unsigned get_uint32() const {
            MERAK_JSON_ASSERT(data_.f.flags & kUintFlag);
            return data_.n.u.u;
        }

        [[nodiscard]] constexpr int64_t get_int64() const {
            MERAK_JSON_ASSERT(data_.f.flags & kInt64Flag);
            return data_.n.i64;
        }

        [[nodiscard]] constexpr uint64_t get_uint64() const {
            MERAK_JSON_ASSERT(data_.f.flags & kUint64Flag);
            return data_.n.u64;
        }

        //! Get the value as double type.
        /*! \note If the value is 64-bit integer type, it may lose precision. Use \c is_lossless_double() to check whether the converison is lossless.
        */
        [[nodiscard]] constexpr double get_double() const {
            MERAK_JSON_ASSERT(is_number());
            if ((data_.f.flags & kDoubleFlag) != 0) return data_.n.d;   // exact type, no conversion.
            if ((data_.f.flags & kIntFlag) != 0) return data_.n.i.i; // int -> double
            if ((data_.f.flags & kUintFlag) != 0) return data_.n.u.u; // unsigned -> double
            if ((data_.f.flags & kInt64Flag) != 0)
                return static_cast<double>(data_.n.i64); // int64_t -> double (may lose precision)
            MERAK_JSON_ASSERT((data_.f.flags & kUint64Flag) != 0);
            return static_cast<double>(data_.n.u64); // uint64_t -> double (may lose precision)
        }

        //! Get the value as float type.
        /*! \note If the value is 64-bit integer type, it may lose precision. Use \c is_lossless_float() to check whether the converison is lossless.
        */
        [[nodiscard]] constexpr float get_float() const {
            return static_cast<float>(get_double());
        }

        GenericValue &set_int32(int i) {
            this->~GenericValue();
            new(this) GenericValue(i);
            return *this;
        }

        GenericValue &set_uint32(unsigned u) {
            this->~GenericValue();
            new(this) GenericValue(u);
            return *this;
        }

        GenericValue &set_int64(int64_t i64) {
            this->~GenericValue();
            new(this) GenericValue(i64);
            return *this;
        }

        GenericValue &set_uint64(uint64_t u64) {
            this->~GenericValue();
            new(this) GenericValue(u64);
            return *this;
        }

        GenericValue &set_double(double d) {
            this->~GenericValue();
            new(this) GenericValue(d);
            return *this;
        }

        GenericValue &set_float(float f) {
            this->~GenericValue();
            new(this) GenericValue(static_cast<double>(f));
            return *this;
        }

        //@}

        //!@name String
        //@{

        [[nodiscard]] const char_type *get_string() const {
            MERAK_JSON_ASSERT(is_string());
            return data_string(data_);
        }

        //! Get the length of string.
        /*! Since rapidjson permits "\\u0000" in the json string, strlen(v.get_string()) may not equal to v.get_string_length().
        */
        [[nodiscard]] SizeType get_string_length() const {
            MERAK_JSON_ASSERT(is_string());
            return data_string_length(data_);
        }

        //! Set this value as a string without copying source string.
        /*! This version has better performance with supplied length, and also support string containing null character.
            \param s source string pointer.
            \param length The length of source string, excluding the trailing null terminator.
            \return The value itself for fluent API.
            \post is_string() == true && get_string() == s && get_string_length() == length
            \see set_string(StringRefType)
        */
        GenericValue &set_string(const char_type *s, SizeType length) { return set_string(StringRef(s, length)); }

        //! Set this value as a string without copying source string.
        /*! \param s source string reference
            \return The value itself for fluent API.
            \post is_string() == true && get_string() == s && get_string_length() == s.length
        */
        GenericValue &set_string(StringRefType s) {
            this->~GenericValue();
            set_string_raw(s);
            return *this;
        }

        //! Set this value as a string by copying from source string.
        /*! This version has better performance with supplied length, and also support string containing null character.
            \param s source string.
            \param length The length of source string, excluding the trailing null terminator.
            \param allocator Allocator for allocating copied buffer. Commonly use GenericDocument::get_allocator().
            \return The value itself for fluent API.
            \post is_string() == true && get_string() != s && strcmp(get_string(),s) == 0 && get_string_length() == length
        */
        GenericValue &set_string(const char_type *s, SizeType length, Allocator &allocator) {
            return set_string(StringRef(s, length), allocator);
        }

        //! Set this value as a string by copying from source string.
        /*! \param s source string.
            \param allocator Allocator for allocating copied buffer. Commonly use GenericDocument::get_allocator().
            \return The value itself for fluent API.
            \post is_string() == true && get_string() != s && strcmp(get_string(),s) == 0 && get_string_length() == length
        */
        GenericValue &set_string(const char_type *s, Allocator &allocator) { return set_string(StringRef(s), allocator); }

        //! Set this value as a string by copying from source string.
        /*! \param s source string reference
            \param allocator Allocator for allocating copied buffer. Commonly use GenericDocument::get_allocator().
            \return The value itself for fluent API.
            \post is_string() == true && get_string() != s.s && strcmp(get_string(),s) == 0 && get_string_length() == length
        */
        GenericValue &set_string(StringRefType s, Allocator &allocator) {
            this->~GenericValue();
            set_string_raw(s, allocator);
            return *this;
        }

        //! Set this value as a string by copying from source string.
        /*! \param s source string.
            \param allocator Allocator for allocating copied buffer. Commonly use GenericDocument::get_allocator().
            \return The value itself for fluent API.
            \post is_string() == true && get_string() != s.data() && strcmp(get_string(),s.data() == 0 && get_string_length() == s.size()
            \note Requires the definition of the preprocessor symbol.
        */
        GenericValue &set_string(const std::basic_string<char_type> &s, Allocator &allocator) {
            return set_string(StringRef(s), allocator);
        }


        //@}

        //!@name Array
        //@{

        //! Templated version for checking whether this value is type T.
        /*!
            \tparam T Either \c bool, \c int, \c unsigned, \c int64_t, \c uint64_t, \c double, \c float, \c const \c char*, \c std::basic_string<char_type>
        */
        template<typename T>
        bool is_type() const { return internal::TypeHelper<ValueType, T>::is_type(*this); }

        template<typename T>
        T get() const { return internal::TypeHelper<ValueType, T>::get(*this); }

        template<typename T>
        T get() { return internal::TypeHelper<ValueType, T>::get(*this); }

        template<typename T>
        ValueType &set(const T &data) { return internal::TypeHelper<ValueType, T>::set(*this, data); }

        template<typename T>
        ValueType &set(const T &data, AllocatorType &allocator) {
            return internal::TypeHelper<ValueType, T>::set(*this, data, allocator);
        }

        void describe(std::ostream &os);

        [[nodiscard]] std::string to_string() const;

        void pretty_describe(std::ostream &os);

        [[nodiscard]] std::string to_pretty_string() const;

        //@}

        //! Generate events of this value to a Handler.
        /*! This function adopts the GoF visitor pattern.
            Typical usage is to output this JSON value as JSON text via Writer, which is a Handler.
            It can also be used to deep clone this value via GenericDocument, which is also a Handler.
            \tparam Handler type of handler.
            \param handler An object implementing concept Handler.
        */
        template<typename Handler>
        bool accept(Handler &handler) const {
            switch (get_type()) {
                case kNullType:
                    return handler.emplace_null();
                case kFalseType:
                    return handler.emplace_bool(false);
                case kTrueType:
                    return handler.emplace_bool(true);

                case kObjectType:
                    if (MERAK_JSON_UNLIKELY(!handler.start_object()))
                        return false;
                    for (ConstMemberIterator m = member_begin(); m != member_end(); ++m) {
                        MERAK_JSON_ASSERT(m->name.is_string()); // User may change the type of name by MemberIterator.
                        if (MERAK_JSON_UNLIKELY(!handler.emplace_key(m->name.get_string(), m->name.get_string_length(),
                                                             (m->name.data_.f.flags & kCopyFlag) != 0)))
                            return false;
                        if (MERAK_JSON_UNLIKELY(!m->value.accept(handler)))
                            return false;
                    }
                    return handler.end_object(data_.o.size);

                case kArrayType:
                    if (MERAK_JSON_UNLIKELY(!handler.start_array()))
                        return false;
                    for (ConstValueIterator v = begin(); v != end(); ++v)
                        if (MERAK_JSON_UNLIKELY(!v->accept(handler)))
                            return false;
                    return handler.end_array(data_.a.size);

                case kStringType:
                    return handler.emplace_string(get_string(), get_string_length(), (data_.f.flags & kCopyFlag) != 0);

                default:
                    MERAK_JSON_ASSERT(get_type() == kNumberType);
                    if (is_double()) return handler.emplace_double(data_.n.d);
                    else if (is_int32()) return handler.emplace_int32(data_.n.i.i);
                    else if (is_uint32()) return handler.emplace_uint32(data_.n.u.u);
                    else if (is_int64()) return handler.emplace_int64(data_.n.i64);
                    else return handler.emplace_uint64(data_.n.u64);
            }
        }

    private:
        template<typename, typename> friend
        class GenericValue;

        template<typename, typename, typename> friend
        class GenericDocument;

        enum {
            kBoolFlag = 0x0008,
            kNumberFlag = 0x0010,
            kIntFlag = 0x0020,
            kUintFlag = 0x0040,
            kInt64Flag = 0x0080,
            kUint64Flag = 0x0100,
            kDoubleFlag = 0x0200,
            kStringFlag = 0x0400,
            kCopyFlag = 0x0800,
            kInlineStrFlag = 0x1000,

            // Initial flags of different types.
            kNullFlag = kNullType,
            // These casts are added to suppress the warning on MSVC about bitwise operations between enums of different types.
            kTrueFlag = static_cast<int>(kTrueType) | static_cast<int>(kBoolFlag),
            kFalseFlag = static_cast<int>(kFalseType) | static_cast<int>(kBoolFlag),
            kNumberIntFlag = static_cast<int>(kNumberType) | static_cast<int>(kNumberFlag | kIntFlag | kInt64Flag),
            kNumberUintFlag =
            static_cast<int>(kNumberType) | static_cast<int>(kNumberFlag | kUintFlag | kUint64Flag | kInt64Flag),
            kNumberInt64Flag = static_cast<int>(kNumberType) | static_cast<int>(kNumberFlag | kInt64Flag),
            kNumberUint64Flag = static_cast<int>(kNumberType) | static_cast<int>(kNumberFlag | kUint64Flag),
            kNumberDoubleFlag = static_cast<int>(kNumberType) | static_cast<int>(kNumberFlag | kDoubleFlag),
            kNumberAnyFlag = static_cast<int>(kNumberType) |
                             static_cast<int>(kNumberFlag | kIntFlag | kInt64Flag | kUintFlag | kUint64Flag |
                                              kDoubleFlag),
            kConstStringFlag = static_cast<int>(kStringType) | static_cast<int>(kStringFlag),
            kCopyStringFlag = static_cast<int>(kStringType) | static_cast<int>(kStringFlag | kCopyFlag),
            kShortStringFlag =
            static_cast<int>(kStringType) | static_cast<int>(kStringFlag | kCopyFlag | kInlineStrFlag),
            kObjectFlag = kObjectType,
            kArrayFlag = kArrayType,

            kTypeMask = 0x07
        };

        static const SizeType kDefaultArrayCapacity = MERAK_JSON_VALUE_DEFAULT_ARRAY_CAPACITY;
        static const SizeType kDefaultObjectCapacity = MERAK_JSON_VALUE_DEFAULT_OBJECT_CAPACITY;

        struct Flag {
#if MERAK_JSON_48BITPOINTER_OPTIMIZATION
            char payload[sizeof(SizeType) * 2 + 6];     // 2 x SizeType + lower 48-bit pointer
#elif MERAK_JSON_64BIT
            char payload[sizeof(SizeType) * 2 + sizeof(void*) + 6]; // 6 padding bytes
#else
            char payload[sizeof(SizeType) * 2 + sizeof(void*) + 2]; // 2 padding bytes
#endif
            uint16_t flags;
        };

        struct String {
            SizeType length;
            SizeType hashcode;  //!< reserved
            const char_type *str;
        };  // 12 bytes in 32-bit mode, 16 bytes in 64-bit mode

        // implementation detail: ShortString can represent zero-terminated strings up to MaxSize chars
        // (excluding the terminating zero) and store a value to determine the length of the contained
        // string in the last character str[LenPos] by storing "MaxSize - length" there. If the string
        // to store has the maximal length of MaxSize then str[LenPos] will be 0 and therefore act as
        // the string terminator as well. For getting the string length back from that value just use
        // "MaxSize - str[LenPos]".
        // This allows to store 13-chars strings in 32-bit mode, 21-chars strings in 64-bit mode,
        // 13-chars strings for MERAK_JSON_48BITPOINTER_OPTIMIZATION=1 inline (for `UTF8`-encoded strings).
        struct ShortString {
            enum {
                MaxChars = sizeof(static_cast<Flag *>(0)->payload) / sizeof(char_type),
                MaxSize = MaxChars - 1,
                LenPos = MaxSize
            };
            char_type str[MaxChars];

            inline static bool usable(SizeType len) { return (MaxSize >= len); }

            inline void set_length(SizeType len) { str[LenPos] = static_cast<char_type>(MaxSize - len); }

            inline SizeType get_length() const { return static_cast<SizeType>(MaxSize - str[LenPos]); }
        };  // at most as many bytes as "String" above => 12 bytes in 32-bit mode, 16 bytes in 64-bit mode

        // By using proper binary layout, retrieval of different integer types do not need conversions.
        union Number {
#if MERAK_JSON_ENDIAN == MERAK_JSON_LITTLEENDIAN
            struct I {
                int i;
                char padding[4];
            } i;
            struct U {
                unsigned u;
                char padding2[4];
            } u;
#else
            struct I {
                char padding[4];
                int i;
            }i;
            struct U {
                char padding2[4];
                unsigned u;
            }u;
#endif
            int64_t i64;
            uint64_t u64;
            double d;
        };  // 8 bytes

        struct ObjectData {
            SizeType size;
            SizeType capacity;
            Member *members;
        };  // 12 bytes in 32-bit mode, 16 bytes in 64-bit mode

        struct ArrayData {
            SizeType size;
            SizeType capacity;
            GenericValue *elements;
        };  // 12 bytes in 32-bit mode, 16 bytes in 64-bit mode

        union Data {
            String s;
            ShortString ss;
            Number n;
            ObjectData o;
            ArrayData a;
            Flag f;
        };  // 16 bytes in 32-bit mode, 24 bytes in 64-bit mode, 16 bytes in 64-bit with MERAK_JSON_48BITPOINTER_OPTIMIZATION

        static MERAK_JSON_FORCEINLINE const char_type *data_string(const Data &data) {
            return (data.f.flags & kInlineStrFlag) ? data.ss.str : MERAK_JSON_GETPOINTER(char_type, data.s.str);
        }

        static MERAK_JSON_FORCEINLINE SizeType data_string_length(const Data &data) {
            return (data.f.flags & kInlineStrFlag) ? data.ss.get_length() : data.s.length;
        }

        MERAK_JSON_FORCEINLINE const char_type *get_string_pointer() const { return MERAK_JSON_GETPOINTER(char_type, data_.s.str); }

        MERAK_JSON_FORCEINLINE const char_type *set_string_pointer(const char_type *str) {
            return MERAK_JSON_SETPOINTER(char_type, data_.s.str, str);
        }

        MERAK_JSON_FORCEINLINE GenericValue *get_elements_pointer() const {
            return MERAK_JSON_GETPOINTER(GenericValue, data_.a.elements);
        }

        MERAK_JSON_FORCEINLINE GenericValue *set_elements_pointer(GenericValue *elements) {
            return MERAK_JSON_SETPOINTER(GenericValue, data_.a.elements, elements);
        }

        MERAK_JSON_FORCEINLINE Member *get_members_pointer() const {
            return MERAK_JSON_GETPOINTER(Member, data_.o.members);
        }

        MERAK_JSON_FORCEINLINE Member *set_members_pointer(Member *members) {
            return MERAK_JSON_SETPOINTER(Member, data_.o.members, members);
        }

#if MERAK_JSON_USE_MEMBERSMAP

        struct MapTraits {
            struct Less {
                bool operator()(const Data& s1, const Data& s2) const {
                    SizeType n1 = data_string_length(s1), n2 = data_string_length(s2);
                    int cmp = std::memcmp(data_string(s1), data_string(s2), sizeof(char_type) * (n1 < n2 ? n1 : n2));
                    return cmp < 0 || (cmp == 0 && n1 < n2);
                }
            };
            typedef std::pair<const Data, SizeType> Pair;
            typedef std::multimap<Data, SizeType, Less, StdAllocator<Pair, Allocator> > Map;
            typedef typename Map::iterator Iterator;
        };
        typedef typename MapTraits::Map         Map;
        typedef typename MapTraits::Less        MapLess;
        typedef typename MapTraits::Pair        MapPair;
        typedef typename MapTraits::Iterator    MapIterator;

        //
        // Layout of the members' map/array, re(al)located according to the needed capacity:
        //
        //    {Map*}<>{capacity}<>{Member[capacity]}<>{MapIterator[capacity]}
        //
        // (where <> stands for the MERAK_JSON_ALIGN-ment, if needed)
        //

        static MERAK_JSON_FORCEINLINE size_t GetMapLayoutSize(SizeType capacity) {
            return MERAK_JSON_ALIGN(sizeof(Map*)) +
                   MERAK_JSON_ALIGN(sizeof(SizeType)) +
                   MERAK_JSON_ALIGN(capacity * sizeof(Member)) +
                   capacity * sizeof(MapIterator);
        }

        static MERAK_JSON_FORCEINLINE SizeType &GetMapCapacity(Map* &map) {
            return *reinterpret_cast<SizeType*>(reinterpret_cast<uintptr_t>(&map) +
                                                MERAK_JSON_ALIGN(sizeof(Map*)));
        }

        static MERAK_JSON_FORCEINLINE Member* GetMapMembers(Map* &map) {
            return reinterpret_cast<Member*>(reinterpret_cast<uintptr_t>(&map) +
                                             MERAK_JSON_ALIGN(sizeof(Map*)) +
                                             MERAK_JSON_ALIGN(sizeof(SizeType)));
        }

        static MERAK_JSON_FORCEINLINE MapIterator* GetMapIterators(Map* &map) {
            return reinterpret_cast<MapIterator*>(reinterpret_cast<uintptr_t>(&map) +
                                                  MERAK_JSON_ALIGN(sizeof(Map*)) +
                                                  MERAK_JSON_ALIGN(sizeof(SizeType)) +
                                                  MERAK_JSON_ALIGN(GetMapCapacity(map) * sizeof(Member)));
        }

        static MERAK_JSON_FORCEINLINE Map* &GetMap(Member* members) {
            MERAK_JSON_ASSERT(members != 0);
            return *reinterpret_cast<Map**>(reinterpret_cast<uintptr_t>(members) -
                                            MERAK_JSON_ALIGN(sizeof(SizeType)) -
                                            MERAK_JSON_ALIGN(sizeof(Map*)));
        }

        // Some compilers' debug mechanisms want all iterators to be destroyed, for their accounting..
        MERAK_JSON_FORCEINLINE MapIterator DropMapIterator(MapIterator& rhs) {
            MapIterator ret = std::move(rhs);
            rhs.~MapIterator();
            return ret;
        }

        Map* &DoReallocMap(Map** oldMap, SizeType newCapacity, Allocator& allocator) {
            Map **newMap = static_cast<Map**>(allocator.Malloc(GetMapLayoutSize(newCapacity)));
            GetMapCapacity(*newMap) = newCapacity;
            if (!oldMap) {
                *newMap = new (allocator.Malloc(sizeof(Map))) Map(MapLess(), allocator);
            }
            else {
                *newMap = *oldMap;
                size_t count = (*oldMap)->size();
                std::memcpy(static_cast<void*>(GetMapMembers(*newMap)),
                            static_cast<void*>(GetMapMembers(*oldMap)),
                            count * sizeof(Member));
                MapIterator *oldIt = GetMapIterators(*oldMap),
                            *newIt = GetMapIterators(*newMap);
                while (count--) {
                    new (&newIt[count]) MapIterator(DropMapIterator(oldIt[count]));
                }
                Allocator::Free(oldMap);
            }
            return *newMap;
        }

        MERAK_JSON_FORCEINLINE Member* do_alloc_members(SizeType capacity, Allocator& allocator) {
            return GetMapMembers(DoReallocMap(0, capacity, allocator));
        }

        void do_reserve_members(SizeType newCapacity, Allocator& allocator) {
            ObjectData& o = data_.o;
            if (newCapacity > o.capacity) {
                Member* oldMembers = get_members_pointer();
                Map **oldMap = oldMembers ? &GetMap(oldMembers) : 0,
                    *&newMap = DoReallocMap(oldMap, newCapacity, allocator);
                MERAK_JSON_SETPOINTER(Member, o.members, GetMapMembers(newMap));
                o.capacity = newCapacity;
            }
        }

        template <typename SourceAllocator>
        MemberIterator do_find_member(const GenericValue<Encoding, SourceAllocator>& name) {
            if (Member* members = get_members_pointer()) {
                Map* &map = GetMap(members);
                MapIterator mit = map->find(reinterpret_cast<const Data&>(name.data_));
                if (mit != map->end()) {
                    return MemberIterator(&members[mit->second]);
                }
            }
            return member_end();
        }

        void do_clear_members() {
            if (Member* members = get_members_pointer()) {
                Map* &map = GetMap(members);
                MapIterator* mit = GetMapIterators(map);
                for (SizeType i = 0; i < data_.o.size; i++) {
                    map->erase(DropMapIterator(mit[i]));
                    members[i].~Member();
                }
                data_.o.size = 0;
            }
        }

        void do_free_members() {
            if (Member* members = get_members_pointer()) {
                GetMap(members)->~Map();
                for (SizeType i = 0; i < data_.o.size; i++) {
                    members[i].~Member();
                }
                if (Allocator::kNeedFree) { // Shortcut by Allocator's trait
                    Map** map = &GetMap(members);
                    Allocator::Free(*map);
                    Allocator::Free(map);
                }
            }
        }

#else // !MERAK_JSON_USE_MEMBERSMAP

        MERAK_JSON_FORCEINLINE Member *do_alloc_members(SizeType capacity, Allocator &allocator) {
            return Malloc<Member>(allocator, capacity);
        }

        void do_reserve_members(SizeType newCapacity, Allocator &allocator) {
            ObjectData &o = data_.o;
            if (newCapacity > o.capacity) {
                Member *newMembers = Realloc<Member>(allocator, get_members_pointer(), o.capacity, newCapacity);
                MERAK_JSON_SETPOINTER(Member, o.members, newMembers);
                o.capacity = newCapacity;
            }
        }

        template<typename SourceAllocator>
        MemberIterator do_find_member(const GenericValue<Encoding, SourceAllocator> &name) {
            MemberIterator member = member_begin();
            for (; member != member_end(); ++member)
                if (name.string_equal(member->name))
                    break;
            return member;
        }

        void do_clear_members() {
            for (MemberIterator m = member_begin(); m != member_end(); ++m)
                m->~Member();
            data_.o.size = 0;
        }

        void do_free_members() {
            for (MemberIterator m = member_begin(); m != member_end(); ++m)
                m->~Member();
            Allocator::Free(get_members_pointer());
        }

#endif // !MERAK_JSON_USE_MEMBERSMAP

        void do_add_member(GenericValue &name, GenericValue &value, Allocator &allocator) {
            ObjectData &o = data_.o;
            if (o.size >= o.capacity)
                do_reserve_members(o.capacity ? (o.capacity + (o.capacity + 1) / 2) : kDefaultObjectCapacity,
                                   allocator);
            Member *members = get_members_pointer();
            Member *m = members + o.size;
            m->name.raw_assign(name);
            m->value.raw_assign(value);
#if MERAK_JSON_USE_MEMBERSMAP
            Map* &map = GetMap(members);
            MapIterator* mit = GetMapIterators(map);
            new (&mit[o.size]) MapIterator(map->insert(MapPair(m->name.data_, o.size)));
#endif
            ++o.size;
        }

        MemberIterator do_remove_member(MemberIterator m) {
            ObjectData &o = data_.o;
            Member *members = get_members_pointer();
#if MERAK_JSON_USE_MEMBERSMAP
            Map* &map = GetMap(members);
            MapIterator* mit = GetMapIterators(map);
            SizeType mpos = static_cast<SizeType>(&*m - members);
            map->erase(DropMapIterator(mit[mpos]));
#endif
            MemberIterator last(members + (o.size - 1));
            if (o.size > 1 && m != last) {
#if MERAK_JSON_USE_MEMBERSMAP
                new (&mit[mpos]) MapIterator(DropMapIterator(mit[&*last - members]));
                mit[mpos]->second = mpos;
#endif
                *m = *last; // Move the last one to this place
            } else {
                m->~Member(); // Only one left, just destroy
            }
            --o.size;
            return m;
        }

        MemberIterator do_erase_members(ConstMemberIterator first, ConstMemberIterator last) {
            ObjectData &o = data_.o;
            MemberIterator beg = member_begin(),
                    pos = beg + (first - beg),
                    end = member_end();
#if MERAK_JSON_USE_MEMBERSMAP
            Map* &map = GetMap(get_members_pointer());
            MapIterator* mit = GetMapIterators(map);
#endif
            for (MemberIterator itr = pos; itr != last; ++itr) {
#if MERAK_JSON_USE_MEMBERSMAP
                map->erase(DropMapIterator(mit[itr - beg]));
#endif
                itr->~Member();
            }
#if MERAK_JSON_USE_MEMBERSMAP
            if (first != last) {
                // Move remaining members/iterators
                MemberIterator next = pos + (last - first);
                for (MemberIterator itr = pos; next != end; ++itr, ++next) {
                    std::memcpy(static_cast<void*>(&*itr), &*next, sizeof(Member));
                    SizeType mpos = static_cast<SizeType>(itr - beg);
                    new (&mit[mpos]) MapIterator(DropMapIterator(mit[next - beg]));
                    mit[mpos]->second = mpos;
                }
            }
#else
            std::memmove(static_cast<void *>(&*pos), &*last,
                         static_cast<size_t>(end - last) * sizeof(Member));
#endif
            o.size -= static_cast<SizeType>(last - first);
            return pos;
        }

        template<typename SourceAllocator>
        void
        do_copy_members(const GenericValue<Encoding, SourceAllocator> &rhs, Allocator &allocator,
                        bool copyConstStrings) {
            MERAK_JSON_ASSERT(rhs.get_type() == kObjectType);

            data_.f.flags = kObjectFlag;
            SizeType count = rhs.data_.o.size;
            Member *lm = do_alloc_members(count, allocator);
            const typename GenericValue<Encoding, SourceAllocator>::Member *rm = rhs.get_members_pointer();
#if MERAK_JSON_USE_MEMBERSMAP
            Map* &map = GetMap(lm);
            MapIterator* mit = GetMapIterators(map);
#endif
            for (SizeType i = 0; i < count; i++) {
                new(&lm[i].name) GenericValue(rm[i].name, allocator, copyConstStrings);
                new(&lm[i].value) GenericValue(rm[i].value, allocator, copyConstStrings);
#if MERAK_JSON_USE_MEMBERSMAP
                new (&mit[i]) MapIterator(map->insert(MapPair(lm[i].name.data_, i)));
#endif
            }
            data_.o.size = data_.o.capacity = count;
            set_members_pointer(lm);
        }

        // Initialize this value as array with initial data, without calling destructor.
        void set_array_raw(GenericValue *values, SizeType count, Allocator &allocator) {
            data_.f.flags = kArrayFlag;
            if (count) {
                GenericValue *e = static_cast<GenericValue *>(allocator.Malloc(count * sizeof(GenericValue)));
                set_elements_pointer(e);
                std::memcpy(static_cast<void *>(e), values, count * sizeof(GenericValue));
            } else
                set_elements_pointer(0);
            data_.a.size = data_.a.capacity = count;
        }

        //! Initialize this value as object with initial data, without calling destructor.
        void set_ubject_raw(Member *members, SizeType count, Allocator &allocator) {
            data_.f.flags = kObjectFlag;
            if (count) {
                Member *m = do_alloc_members(count, allocator);
                set_members_pointer(m);
                std::memcpy(static_cast<void *>(m), members, count * sizeof(Member));
#if MERAK_JSON_USE_MEMBERSMAP
                Map* &map = GetMap(m);
                MapIterator* mit = GetMapIterators(map);
                for (SizeType i = 0; i < count; i++) {
                    new (&mit[i]) MapIterator(map->insert(MapPair(m[i].name.data_, i)));
                }
#endif
            } else
                set_members_pointer(0);
            data_.o.size = data_.o.capacity = count;
        }

        //! Initialize this value as constant string, without calling destructor.
        void set_string_raw(StringRefType s) MERAK_JSON_NOEXCEPT {
            data_.f.flags = kConstStringFlag;
            set_string_pointer(s);
            data_.s.length = s.length;
        }

        //! Initialize this value as copy string with initial data, without calling destructor.
        void set_string_raw(StringRefType s, Allocator &allocator) {
            char_type *str = 0;
            if (ShortString::usable(s.length)) {
                data_.f.flags = kShortStringFlag;
                data_.ss.set_length(s.length);
                str = data_.ss.str;
                std::memmove(str, s, s.length * sizeof(char_type));
            } else {
                data_.f.flags = kCopyStringFlag;
                data_.s.length = s.length;
                str = static_cast<char_type *>(allocator.Malloc((s.length + 1) * sizeof(char_type)));
                set_string_pointer(str);
                std::memcpy(str, s, s.length * sizeof(char_type));
            }
            str[s.length] = '\0';
        }

        //! Assignment without calling destructor
        void raw_assign(GenericValue &rhs) MERAK_JSON_NOEXCEPT {
            data_ = rhs.data_;
            // data_.f.flags = rhs.data_.f.flags;
            rhs.data_.f.flags = kNullFlag;
        }

        template<typename SourceAllocator>
        bool string_equal(const GenericValue<Encoding, SourceAllocator> &rhs) const {
            MERAK_JSON_ASSERT(is_string());
            MERAK_JSON_ASSERT(rhs.is_string());

            const SizeType len1 = get_string_length();
            const SizeType len2 = rhs.get_string_length();
            if (len1 != len2) { return false; }

            const char_type *const str1 = get_string();
            const char_type *const str2 = rhs.get_string();
            if (str1 == str2) { return true; } // fast path for constant string

            return (std::memcmp(str1, str2, sizeof(char_type) * len1) == 0);
        }

        Data data_;
    };

    //! GenericValue with UTF8 encoding
    typedef GenericValue<UTF8<> > Value;

    ///////////////////////////////////////////////////////////////////////////////
    // GenericDocument

    //! A document for parsing JSON text as DOM.
    /*!
        \note implements Handler concept
        \tparam Encoding Encoding for both parsing and string storage.
        \tparam Allocator Allocator for allocating memory for the DOM
        \tparam StackAllocator Allocator for allocating memory for stack during parsing.
        \warning Although GenericDocument inherits from GenericValue, the API does \b not provide any virtual functions, especially no virtual destructor.  To avoid memory leaks, do not \c delete a GenericDocument object via a pointer to a GenericValue.
    */
    template<typename Encoding, typename Allocator = MERAK_JSON_DEFAULT_ALLOCATOR, typename StackAllocator = MERAK_JSON_DEFAULT_STACK_ALLOCATOR>
    class GenericDocument : public GenericValue<Encoding, Allocator> {
    public:
        typedef typename Encoding::char_type char_type;                       //!< Character type derived from Encoding.
        typedef GenericValue<Encoding, Allocator> ValueType;    //!< Value type of the document.
        typedef Allocator AllocatorType;                        //!< Allocator type from template parameter.
        typedef StackAllocator StackAllocatorType;              //!< StackAllocator type from template parameter.

        //! Constructor
        /*! Creates an empty document of specified type.
            \param type             Mandatory type of object to create.
            \param allocator        Optional allocator for allocating memory.
            \param stackCapacity    Optional initial capacity of stack in bytes.
            \param stackAllocator   Optional allocator for allocating memory for stack.
        */
        explicit GenericDocument(Type type, Allocator *allocator = 0, size_t stackCapacity = kDefaultStackCapacity,
                                 StackAllocator *stackAllocator = 0) :
                GenericValue<Encoding, Allocator>(type), allocator_(allocator), ownAllocator_(0),
                stack_(stackAllocator, stackCapacity), parseResult_() {
            if (!allocator_)
                ownAllocator_ = allocator_ = MERAK_JSON_NEW(Allocator)();
        }

        //! Constructor
        /*! Creates an empty document which type is Null.
            \param allocator        Optional allocator for allocating memory.
            \param stackCapacity    Optional initial capacity of stack in bytes.
            \param stackAllocator   Optional allocator for allocating memory for stack.
        */
        GenericDocument(Allocator *allocator = 0, size_t stackCapacity = kDefaultStackCapacity,
                        StackAllocator *stackAllocator = 0) :
                allocator_(allocator), ownAllocator_(0), stack_(stackAllocator, stackCapacity), parseResult_() {
            if (!allocator_)
                ownAllocator_ = allocator_ = MERAK_JSON_NEW(Allocator)();
        }


        //! Move constructor in C++11
        GenericDocument(GenericDocument &&rhs) MERAK_JSON_NOEXCEPT
                : ValueType(std::forward<ValueType>(rhs)), // explicit cast to avoid prohibited move from Document
                  allocator_(rhs.allocator_),
                  ownAllocator_(rhs.ownAllocator_),
                  stack_(std::move(rhs.stack_)),
                  parseResult_(rhs.parseResult_) {
            rhs.allocator_ = 0;
            rhs.ownAllocator_ = 0;
            rhs.parseResult_ = ParseResult();
        }


        ~GenericDocument() {
            // clear the ::ValueType before ownAllocator is destroyed, ~ValueType()
            // runs last and may access its elements or members which would be freed
            // with an allocator like MemoryPoolAllocator (CrtAllocator does not
            // free its data when destroyed, but MemoryPoolAllocator does).
            if (ownAllocator_) {
                ValueType::SetNull();
            }
            destroy();
        }


        //! Move assignment in C++11
        GenericDocument &operator=(GenericDocument &&rhs) MERAK_JSON_NOEXCEPT {
            // The cast to ValueType is necessary here, because otherwise it would
            // attempt to call GenericValue's templated assignment operator.
            ValueType::operator=(std::forward<ValueType>(rhs));

            // Calling the destructor here would prematurely call stack_'s destructor
            destroy();

            allocator_ = rhs.allocator_;
            ownAllocator_ = rhs.ownAllocator_;
            stack_ = std::move(rhs.stack_);
            parseResult_ = rhs.parseResult_;

            rhs.allocator_ = 0;
            rhs.ownAllocator_ = 0;
            rhs.parseResult_ = ParseResult();

            return *this;
        }


        //! Exchange the contents of this document with those of another.
        /*!
            \param rhs Another document.
            \note Constant complexity.
            \see GenericValue::swap
        */
        GenericDocument &swap(GenericDocument &rhs) MERAK_JSON_NOEXCEPT {
            ValueType::swap(rhs);
            stack_.swap(rhs.stack_);
            internal::swap(allocator_, rhs.allocator_);
            internal::swap(ownAllocator_, rhs.ownAllocator_);
            internal::swap(parseResult_, rhs.parseResult_);
            return *this;
        }

        // Allow swap with ValueType.
        // Refer to Effective C++ 3rd Edition/Item 33: Avoid hiding inherited names.
        using ValueType::swap;

        //! free-standing swap function helper
        /*!
            Helper function to enable support for common swap implementation pattern based on \c std::swap:
            \code
            void swap(MyClass& a, MyClass& b) {
                using std::swap;
                swap(a.doc, b.doc);
                // ...
            }
            \endcode
            \see swap()
         */
        friend inline void swap(GenericDocument &a, GenericDocument &b) MERAK_JSON_NOEXCEPT { a.swap(b); }

        //! populate this document by a generator which produces SAX events.
        /*! \tparam Generator A functor with <tt>bool f(Handler)</tt> prototype.
            \param g Generator functor which sends SAX events to the parameter.
            \return The document itself for fluent API.
        */
        template<typename Generator>
        GenericDocument &populate(Generator &g) {
            ClearStackOnExit scope(*this);
            if (g(*this)) {
                MERAK_JSON_ASSERT(stack_.GetSize() == sizeof(ValueType)); // Got one and only one root object
                ValueType::operator=(*stack_.template Pop<ValueType>(1));// Move value from stack to document
            }
            return *this;
        }

        //!@name parse from stream
        //!@{

        //! parse JSON text from an input stream (with Encoding conversion)
        /*! \tparam parseFlags Combination of \ref ParseFlag.
            \tparam SourceEncoding Encoding of input stream
            \tparam InputStream Type of input stream, implementing Stream concept
            \param is Input stream to be parsed.
            \return The document itself for fluent API.
        */
        template<unsigned parseFlags, typename SourceEncoding, typename InputStream>
        GenericDocument &parse_stream(InputStream &is) {
            GenericReader<SourceEncoding, Encoding, StackAllocator> reader(
                    stack_.HasAllocator() ? &stack_.get_allocator() : 0);
            ClearStackOnExit scope(*this);
            parseResult_ = reader.template parse<parseFlags>(is, *this);
            if (parseResult_) {
                MERAK_JSON_ASSERT(stack_.GetSize() == sizeof(ValueType)); // Got one and only one root object
                ValueType::operator=(*stack_.template Pop<ValueType>(1));// Move value from stack to document
            }
            return *this;
        }

        //! parse JSON text from an input stream
        /*! \tparam parseFlags Combination of \ref ParseFlag.
            \tparam InputStream Type of input stream, implementing Stream concept
            \param is Input stream to be parsed.
            \return The document itself for fluent API.
        */
        template<unsigned parseFlags, typename InputStream>
        GenericDocument &parse_stream(InputStream &is) {
            return parse_stream < parseFlags, Encoding, InputStream > (is);
        }

        //! parse JSON text from an input stream (with \ref kParseDefaultFlags)
        /*! \tparam InputStream Type of input stream, implementing Stream concept
            \param is Input stream to be parsed.
            \return The document itself for fluent API.
        */
        template<typename InputStream>
        GenericDocument &parse_stream(InputStream &is) {
            return parse_stream < kParseDefaultFlags, Encoding, InputStream > (is);
        }
        //!@}

        //!@name parse from read-only string
        //!@{

        //! parse JSON text from a read-only string (with Encoding conversion)
        /*! \tparam parseFlags Combination of \ref ParseFlag.
            \tparam SourceEncoding Transcoding from input Encoding
            \param str Read-only zero-terminated string to be parsed.
        */
        template<unsigned parseFlags, typename SourceEncoding>
        GenericDocument &parse(const typename SourceEncoding::char_type *str) {
            GenericStringStream<SourceEncoding> s(str);
            return parse_stream<parseFlags, SourceEncoding>(s);
        }

        //! parse JSON text from a read-only string
        /*! \tparam parseFlags Combination of \ref ParseFlag.
            \param str Read-only zero-terminated string to be parsed.
        */
        template<unsigned parseFlags>
        GenericDocument &parse(const char_type *str) {
            return parse < parseFlags, Encoding > (str);
        }

        //! parse JSON text from a read-only string (with \ref kParseDefaultFlags)
        /*! \param str Read-only zero-terminated string to be parsed.
        */
        GenericDocument &parse(const char_type *str) {
            return parse < kParseDefaultFlags > (str);
        }

        template<unsigned parseFlags, typename SourceEncoding>
        GenericDocument &parse(const typename SourceEncoding::char_type *str, size_t length) {
            MemoryStream ms(reinterpret_cast<const char *>(str), length * sizeof(typename SourceEncoding::char_type));
            EncodedInputStream<SourceEncoding, MemoryStream> is(ms);
            parse_stream<parseFlags, SourceEncoding>(is);
            return *this;
        }

        template<unsigned parseFlags>
        GenericDocument &parse(const char_type *str, size_t length) {
            return parse < parseFlags, Encoding > (str, length);
        }

        GenericDocument &parse(const char_type *str, size_t length) {
            return parse < kParseDefaultFlags > (str, length);
        }

        template<unsigned parseFlags, typename SourceEncoding>
        GenericDocument &parse(const std::basic_string<typename SourceEncoding::char_type> &str) {
            // c_str() is constant complexity according to standard. Should be faster than parse(const char*, size_t)
            return parse < parseFlags, SourceEncoding > (str.c_str());
        }

        template<unsigned parseFlags>
        GenericDocument &parse(const std::basic_string<char_type> &str) {
            return parse < parseFlags, Encoding > (str.c_str());
        }

        GenericDocument &parse(const std::basic_string<char_type> &str) {
            return parse < kParseDefaultFlags > (str);
        }

        //!@}

        //!@name Handling parse errors
        //!@{

        //! Whether a parse error has occurred in the last parsing.
        bool has_parse_error() const { return parseResult_.IsError(); }

        //! Get the \ref ParseErrorCode of last parsing.
        ParseErrorCode get_parse_error() const { return parseResult_.Code(); }

        //! Get the position of last parsing error in input, 0 otherwise.
        size_t get_error_offset() const { return parseResult_.Offset(); }

        //! Implicit conversion to get the last parse result
#ifndef __clang // -Wdocumentation
        /*! \return \ref ParseResult of the last parse operation

            \code
              Document doc;
              ParseResult ok = doc.parse(json);
              if (!ok)
                printf( "JSON parse error: %s (%u)\n", get_parse_error_en(ok.Code()), ok.Offset());
            \endcode
         */
#endif

        operator ParseResult() const { return parseResult_; }
        //!@}

        //! Get the allocator of this document.
        Allocator &get_allocator() {
            MERAK_JSON_ASSERT(allocator_);
            return *allocator_;
        }

        //! Get the capacity of stack in bytes.
        size_t get_stack_capacity() const { return stack_.GetCapacity(); }

    private:
        // clear stack on any exit from parse_stream, e.g. due to exception
        struct ClearStackOnExit {
            explicit ClearStackOnExit(GenericDocument &d) : d_(d) {}

            ~ClearStackOnExit() { d_.clear_stack(); }

        private:
            ClearStackOnExit(const ClearStackOnExit &);

            ClearStackOnExit &operator=(const ClearStackOnExit &);

            GenericDocument &d_;
        };

        // callers of the following private Handler functions
        // template <typename,typename,typename> friend class GenericReader; // for parsing
        template<typename, typename> friend
        class GenericValue; // for deep copying

    public:
        // Implementation of Handler
        bool emplace_null() {
            new(stack_.template Push<ValueType>()) ValueType();
            return true;
        }

        bool emplace_bool(bool b) {
            new(stack_.template Push<ValueType>()) ValueType(b);
            return true;
        }

        bool emplace_int32(int i) {
            new(stack_.template Push<ValueType>()) ValueType(i);
            return true;
        }

        bool emplace_uint32(unsigned i) {
            new(stack_.template Push<ValueType>()) ValueType(i);
            return true;
        }

        bool emplace_int64(int64_t i) {
            new(stack_.template Push<ValueType>()) ValueType(i);
            return true;
        }

        bool emplace_uint64(uint64_t i) {
            new(stack_.template Push<ValueType>()) ValueType(i);
            return true;
        }

        bool emplace_double(double d) {
            new(stack_.template Push<ValueType>()) ValueType(d);
            return true;
        }

        bool raw_number(const char_type *str, SizeType length, bool copy) {
            if (copy)
                new(stack_.template Push<ValueType>()) ValueType(str, length, get_allocator());
            else
                new(stack_.template Push<ValueType>()) ValueType(str, length);
            return true;
        }

        bool emplace_string(const char_type *str, SizeType length, bool copy) {
            if (copy)
                new(stack_.template Push<ValueType>()) ValueType(str, length, get_allocator());
            else
                new(stack_.template Push<ValueType>()) ValueType(str, length);
            return true;
        }

        bool start_object() {
            new(stack_.template Push<ValueType>()) ValueType(kObjectType);
            return true;
        }

        bool emplace_key(const char_type *str, SizeType length, bool copy) { return emplace_string(str, length, copy); }

        bool end_object(SizeType memberCount) {
            typename ValueType::Member *members = stack_.template Pop<typename ValueType::Member>(memberCount);
            stack_.template Top<ValueType>()->set_ubject_raw(members, memberCount, get_allocator());
            return true;
        }

        bool start_array() {
            new(stack_.template Push<ValueType>()) ValueType(kArrayType);
            return true;
        }

        bool end_array(SizeType elementCount) {
            ValueType *elements = stack_.template Pop<ValueType>(elementCount);
            stack_.template Top<ValueType>()->set_array_raw(elements, elementCount, get_allocator());
            return true;
        }

    private:
        //! Prohibit copying
        GenericDocument(const GenericDocument &);

        //! Prohibit assignment
        GenericDocument &operator=(const GenericDocument &);

        void clear_stack() {
            if (Allocator::kNeedFree)
                while (stack_.GetSize() >
                       0)    // Here assumes all elements in stack array are GenericValue (Member is actually 2 GenericValue objects)
                    (stack_.template Pop<ValueType>(1))->~ValueType();
            else
                stack_.clear();
            stack_.ShrinkToFit();
        }

        void destroy() {
            MERAK_JSON_DELETE(ownAllocator_);
        }

        static const size_t kDefaultStackCapacity = 1024;
        Allocator *allocator_;
        Allocator *ownAllocator_;
        internal::Stack<StackAllocator> stack_;
        ParseResult parseResult_;
    };

    //! GenericDocument with UTF8 encoding
    typedef GenericDocument<UTF8<> > Document;


    //! Helper class for accessing Value of array type.
    /*!
        Instance of this helper class is obtained by \c GenericValue::get_array().
        In addition to all APIs for array type, it provides range-based for loop if \c=1.
    */
    template<bool Const, typename ValueT>
    class GenericArray {
    public:
        typedef GenericArray<true, ValueT> ConstArray;
        typedef GenericArray<false, ValueT> Array;
        typedef ValueT PlainType;
        typedef typename internal::MaybeAddConst<Const, PlainType>::Type ValueType;
        typedef ValueType *ValueIterator;  // This may be const or non-const iterator
        typedef const ValueT *ConstValueIterator;
        typedef typename ValueType::AllocatorType AllocatorType;
        typedef typename ValueType::StringRefType StringRefType;

        template<typename, typename>
        friend
        class GenericValue;

        GenericArray(const GenericArray &rhs) : value_(rhs.value_) {}

        GenericArray &operator=(const GenericArray &rhs) {
            value_ = rhs.value_;
            return *this;
        }

        ~GenericArray() {}

        operator ValueType &() const { return value_; }

        [[nodiscard]] SizeType size() const { return value_.size(); }

        [[nodiscard]] SizeType capacity() const { return value_.capacity(); }

        [[nodiscard]] bool empty() const { return value_.empty(); }

        void clear() const { value_.clear(); }

        ValueType &operator[](SizeType index) const { return value_[index]; }

        ValueIterator begin() const { return value_.begin(); }

        ValueIterator end() const { return value_.end(); }

        GenericArray reserve(SizeType newCapacity, AllocatorType &allocator) const {
            value_.reserve(newCapacity, allocator);
            return *this;
        }

        GenericArray push_back(ValueType &value, AllocatorType &allocator) const {
            value_.push_back(value, allocator);
            return *this;
        }


        GenericArray push_back(ValueType &&value, AllocatorType &allocator) const {
            value_.push_back(value, allocator);
            return *this;
        }


        GenericArray push_back(StringRefType value, AllocatorType &allocator) const {
            value_.push_back(value, allocator);
            return *this;
        }

        template<typename T>
        MERAK_JSON_DISABLEIF_RETURN((internal::OrExpr<internal::IsPointer<T>, internal::IsGenericValue<T> >),
                                    (const GenericArray&)) push_back(T value, AllocatorType &allocator) const {
            value_.push_back(value, allocator);
            return *this;
        }

        GenericArray pop_back() const {
            value_.pop_back();
            return *this;
        }

        ValueIterator erase(ConstValueIterator pos) const { return value_.erase(pos); }

        ValueIterator erase(ConstValueIterator first, ConstValueIterator last) const {
            return value_.erase(first, last);
        }


    private:
        GenericArray();

        GenericArray(ValueType &value) : value_(value) {}

        ValueType &value_;
    };

    //! Helper class for accessing Value of object type.
    template<bool Const, typename ValueT>
    class GenericObject {
    public:
        typedef GenericObject<true, ValueT> ConstObject;
        typedef GenericObject<false, ValueT> Object;
        typedef ValueT PlainType;
        typedef typename internal::MaybeAddConst<Const, PlainType>::Type ValueType;
        typedef GenericMemberIterator<Const, typename ValueT::EncodingType, typename ValueT::AllocatorType> MemberIterator;  // This may be const or non-const iterator
        typedef GenericMemberIterator<true, typename ValueT::EncodingType, typename ValueT::AllocatorType> ConstMemberIterator;
        typedef typename ValueType::AllocatorType AllocatorType;
        typedef typename ValueType::StringRefType StringRefType;
        typedef typename ValueType::EncodingType EncodingType;
        typedef typename ValueType::char_type char_type;

        template<typename, typename>
        friend
        class GenericValue;

        GenericObject(const GenericObject &rhs) : value_(rhs.value_) {}

        GenericObject &operator=(const GenericObject &rhs) {
            value_ = rhs.value_;
            return *this;
        }

        ~GenericObject() {}

        operator ValueType &() const { return value_; }

        SizeType member_count() const { return value_.member_count(); }

        SizeType member_capacity() const { return value_.member_capacity(); }

        bool object_empty() const { return value_.object_empty(); }

        template<typename T>
        ValueType &operator[](T *name) const { return value_[name]; }

        template<typename SourceAllocator>
        ValueType &operator[](const GenericValue<EncodingType, SourceAllocator> &name) const { return value_[name]; }


        ValueType &operator[](const std::basic_string<char_type> &name) const { return value_[name]; }


        MemberIterator member_begin() const { return value_.member_begin(); }

        MemberIterator member_end() const { return value_.member_end(); }

        GenericObject member_reserve(SizeType newCapacity, AllocatorType &allocator) const {
            value_.member_reserve(newCapacity, allocator);
            return *this;
        }

        bool has_member(const char_type *name) const { return value_.has_member(name); }


        bool has_member(const std::basic_string<char_type> &name) const { return value_.has_member(name); }


        template<typename SourceAllocator>
        bool has_member(const GenericValue<EncodingType, SourceAllocator> &name) const {
            return value_.has_member(name);
        }

        MemberIterator find_member(const char_type *name) const { return value_.find_member(name); }

        template<typename SourceAllocator>
        MemberIterator
        find_member(const GenericValue<EncodingType, SourceAllocator> &name) const { return value_.find_member(name); }


        MemberIterator find_member(const std::basic_string<char_type> &name) const { return value_.find_member(name); }


        GenericObject add_member(ValueType &name, ValueType &value, AllocatorType &allocator) const {
            value_.add_member(name, value, allocator);
            return *this;
        }

        GenericObject add_member(ValueType &name, StringRefType value, AllocatorType &allocator) const {
            value_.add_member(name, value, allocator);
            return *this;
        }


        GenericObject add_member(ValueType &name, std::basic_string<char_type> &value, AllocatorType &allocator) const {
            value_.add_member(name, value, allocator);
            return *this;
        }


        template<typename T>
        MERAK_JSON_DISABLEIF_RETURN((internal::OrExpr<internal::IsPointer<T>, internal::IsGenericValue<T> >),
                                    (ValueType & ))
        add_member(ValueType &name, T value, AllocatorType &allocator) const {
            value_.add_member(name, value, allocator);
            return *this;
        }

        GenericObject add_member(ValueType &&name, ValueType &&value, AllocatorType &allocator) const {
            value_.add_member(name, value, allocator);
            return *this;
        }

        GenericObject add_member(ValueType &&name, ValueType &value, AllocatorType &allocator) const {
            value_.add_member(name, value, allocator);
            return *this;
        }

        GenericObject add_member(ValueType &name, ValueType &&value, AllocatorType &allocator) const {
            value_.add_member(name, value, allocator);
            return *this;
        }

        GenericObject add_member(StringRefType name, ValueType &&value, AllocatorType &allocator) const {
            value_.add_member(name, value, allocator);
            return *this;
        }


        GenericObject add_member(StringRefType name, ValueType &value, AllocatorType &allocator) const {
            value_.add_member(name, value, allocator);
            return *this;
        }

        GenericObject add_member(StringRefType name, StringRefType value, AllocatorType &allocator) const {
            value_.add_member(name, value, allocator);
            return *this;
        }

        template<typename T>
        MERAK_JSON_DISABLEIF_RETURN((internal::OrExpr<internal::IsPointer<T>, internal::IsGenericValue<T> >),
                                    (GenericObject))
        add_member(StringRefType name, T value, AllocatorType &allocator) const {
            value_.add_member(name, value, allocator);
            return *this;
        }

        void RemoveAllMembers() { value_.RemoveAllMembers(); }

        bool remove_member(const char_type *name) const { return value_.remove_member(name); }


        bool remove_member(const std::basic_string<char_type> &name) const { return value_.remove_member(name); }


        template<typename SourceAllocator>
        bool remove_member(const GenericValue<EncodingType, SourceAllocator> &name) const {
            return value_.remove_member(name);
        }

        MemberIterator remove_member(MemberIterator m) const { return value_.remove_member(m); }

        MemberIterator erase_member(ConstMemberIterator pos) const { return value_.erase_member(pos); }

        MemberIterator erase_member(ConstMemberIterator first, ConstMemberIterator last) const {
            return value_.erase_member(first, last);
        }

        bool erase_member(const char_type *name) const { return value_.erase_member(name); }

        bool erase_member(const std::basic_string<char_type> &name) const { return erase_member(ValueType(StringRef(name))); }


        template<typename SourceAllocator>
        bool erase_member(const GenericValue<EncodingType, SourceAllocator> &name) const {
            return value_.erase_member(name);
        }


        MemberIterator begin() const { return value_.member_begin(); }

        MemberIterator end() const { return value_.member_end(); }

    private:
        GenericObject();

        GenericObject(ValueType &value) : value_(value) {}

        ValueType &value_;
    };

}  // namespace merak::json
MERAK_JSON_DIAG_POP

#endif // MERAK_JSON_DOCUMENT_H_
