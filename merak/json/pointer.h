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

#ifndef MERAK_JSON_POINTER_H_
#define MERAK_JSON_POINTER_H_

#include <merak/json/document.h>
#include <merak/json/uri.h>
#include <merak/json/error/error.h>
#include <merak/json/internal/itoa.h>

#ifdef __clang__
MERAK_JSON_DIAG_PUSH
MERAK_JSON_DIAG_OFF(switch-enum)
#elif defined(_MSC_VER)
MERAK_JSON_DIAG_PUSH
MERAK_JSON_DIAG_OFF(4512) // assignment operator could not be generated
#endif

namespace merak::json {

static const SizeType kPointerInvalidIndex = ~SizeType(0);  //!< Represents an invalid index in GenericPointer::Token

///////////////////////////////////////////////////////////////////////////////
// GenericPointer

//! Represents a JSON Pointer. Use Pointer for UTF8 encoding and default allocator.
/*!
    This class implements RFC 6901 "JavaScript Object Notation (JSON) Pointer" 
    (https://tools.ietf.org/html/rfc6901).

    A JSON pointer is for identifying a specific value in a JSON document
    (GenericDocument). It can simplify coding of DOM tree manipulation, because it
    can access multiple-level depth of DOM tree with single API call.

    After it parses a string representation (e.g. "/foo/0" or URI fragment 
    representation (e.g. "#/foo/0") into its internal representation (tokens),
    it can be used to resolve a specific value in multiple documents, or sub-tree 
    of documents.

    Contrary to GenericValue, Pointer can be copy constructed and copy assigned.
    Apart from assignment, a Pointer cannot be modified after construction.

    Although Pointer is very convenient, please aware that constructing Pointer
    involves parsing and dynamic memory allocation. A special constructor with user-
    supplied tokens eliminates these.

    GenericPointer depends on GenericDocument and GenericValue.

    \tparam ValueType The value type of the DOM tree. E.g. GenericValue<UTF8<> >
    \tparam Allocator The allocator type for allocating memory for internal representation.

    \note GenericPointer uses same encoding of ValueType.
    However, Allocator of GenericPointer is independent of Allocator of Value.
*/
template <typename ValueType, typename Allocator = CrtAllocator>
class GenericPointer {
public:
    typedef typename ValueType::EncodingType EncodingType;  //!< Encoding type from Value
    typedef typename ValueType::char_type char_type;                      //!< Character type from Value
    typedef GenericUri<ValueType, Allocator> UriType;


  //! A token is the basic units of internal representation.
    /*!
        A JSON pointer string representation "/foo/123" is parsed to two tokens: 
        "foo" and 123. 123 will be represented in both numeric form and string form.
        They are resolved according to the actual value type (object or array).

        For token that are not numbers, or the numeric value is out of bound
        (greater than limits of SizeType), they are only treated as string form
        (i.e. the token's index will be equal to kPointerInvalidIndex).

        This struct is public so that user can create a Pointer without parsing and 
        allocation, using a special constructor.
    */
    struct Token {
        const char_type* name;             //!< Name of the token. It has null character at the end but it can contain null character.
        SizeType length;            //!< Length of the name.
        SizeType index;             //!< A valid array index, if it is not equal to kPointerInvalidIndex.
    };

    //!@name Constructors and destructor.
    //@{

    //! Default constructor.
    GenericPointer(Allocator* allocator = 0) : allocator_(allocator), ownAllocator_(), nameBuffer_(), tokens_(), tokenCount_(), parseErrorOffset_(), parseErrorCode_(kPointerParseErrorNone) {}

    //! Constructor that parses a string or URI fragment representation.
    /*!
        \param source A null-terminated, string or URI fragment representation of JSON pointer.
        \param allocator User supplied allocator for this pointer. If no allocator is provided, it creates a self-owned one.
    */
    explicit GenericPointer(const char_type* source, Allocator* allocator = 0) : allocator_(allocator), ownAllocator_(), nameBuffer_(), tokens_(), tokenCount_(), parseErrorOffset_(), parseErrorCode_(kPointerParseErrorNone) {
        parse(source, internal::StrLen(source));
    }

    //! Constructor that parses a string or URI fragment representation.
    /*!
        \param source A string or URI fragment representation of JSON pointer.
        \param allocator User supplied allocator for this pointer. If no allocator is provided, it creates a self-owned one.
        \note Requires the definition of the preprocessor symbol.
    */
    explicit GenericPointer(const std::basic_string<char_type>& source, Allocator* allocator = 0) : allocator_(allocator), ownAllocator_(), nameBuffer_(), tokens_(), tokenCount_(), parseErrorOffset_(), parseErrorCode_(kPointerParseErrorNone) {
        parse(source.c_str(), source.size());
    }

    //! Constructor that parses a string or URI fragment representation, with length of the source string.
    /*!
        \param source A string or URI fragment representation of JSON pointer.
        \param length Length of source.
        \param allocator User supplied allocator for this pointer. If no allocator is provided, it creates a self-owned one.
        \note Slightly faster than the overload without length.
    */
    GenericPointer(const char_type* source, size_t length, Allocator* allocator = 0) : allocator_(allocator), ownAllocator_(), nameBuffer_(), tokens_(), tokenCount_(), parseErrorOffset_(), parseErrorCode_(kPointerParseErrorNone) {
        parse(source, length);
    }

    //! Constructor with user-supplied tokens.
    /*!
        This constructor let user supplies const array of tokens.
        This prevents the parsing process and eliminates allocation.
        This is preferred for memory constrained environments.

        \param tokens An constant array of tokens representing the JSON pointer.
        \param tokenCount Number of tokens.

        \b Example
        \code
        #define NAME(s) { s, sizeof(s) / sizeof(s[0]) - 1, kPointerInvalidIndex }
        #define INDEX(i) { #i, sizeof(#i) - 1, i }

        static const Pointer::Token kTokens[] = { NAME("foo"), INDEX(123) };
        static const Pointer p(kTokens, sizeof(kTokens) / sizeof(kTokens[0]));
        // Equivalent to static const Pointer p("/foo/123");

        #undef NAME
        #undef INDEX
        \endcode
    */
    GenericPointer(const Token* tokens, size_t tokenCount) : allocator_(), ownAllocator_(), nameBuffer_(), tokens_(const_cast<Token*>(tokens)), tokenCount_(tokenCount), parseErrorOffset_(), parseErrorCode_(kPointerParseErrorNone) {}

    //! Copy constructor.
    GenericPointer(const GenericPointer& rhs) : allocator_(), ownAllocator_(), nameBuffer_(), tokens_(), tokenCount_(), parseErrorOffset_(), parseErrorCode_(kPointerParseErrorNone) {
        *this = rhs;
    }

    //! Copy constructor.
    GenericPointer(const GenericPointer& rhs, Allocator* allocator) : allocator_(allocator), ownAllocator_(), nameBuffer_(), tokens_(), tokenCount_(), parseErrorOffset_(), parseErrorCode_(kPointerParseErrorNone) {
        *this = rhs;
    }

    //! Destructor.
    ~GenericPointer() {
        if (nameBuffer_)    // If user-supplied tokens constructor is used, nameBuffer_ is nullptr and tokens_ are not deallocated.
            Allocator::Free(tokens_);
        MERAK_JSON_DELETE(ownAllocator_);
    }

    //! Assignment operator.
    GenericPointer& operator=(const GenericPointer& rhs) {
        if (this != &rhs) {
            // Do not delete ownAllcator
            if (nameBuffer_)
                Allocator::Free(tokens_);

            tokenCount_ = rhs.tokenCount_;
            parseErrorOffset_ = rhs.parseErrorOffset_;
            parseErrorCode_ = rhs.parseErrorCode_;

            if (rhs.nameBuffer_)
                CopyFromRaw(rhs); // Normally parsed tokens.
            else {
                tokens_ = rhs.tokens_; // User supplied const tokens.
                nameBuffer_ = 0;
            }
        }
        return *this;
    }

    //! swap the content of this pointer with an other.
    /*!
        \param other The pointer to swap with.
        \note Constant complexity.
    */
    GenericPointer& swap(GenericPointer& other) MERAK_JSON_NOEXCEPT {
        internal::swap(allocator_, other.allocator_);
        internal::swap(ownAllocator_, other.ownAllocator_);
        internal::swap(nameBuffer_, other.nameBuffer_);
        internal::swap(tokens_, other.tokens_);
        internal::swap(tokenCount_, other.tokenCount_);
        internal::swap(parseErrorOffset_, other.parseErrorOffset_);
        internal::swap(parseErrorCode_, other.parseErrorCode_);
        return *this;
    }

    //! free-standing swap function helper
    /*!
        Helper function to enable support for common swap implementation pattern based on \c std::swap:
        \code
        void swap(MyClass& a, MyClass& b) {
            using std::swap;
            swap(a.pointer, b.pointer);
            // ...
        }
        \endcode
        \see swap()
     */
    friend inline void swap(GenericPointer& a, GenericPointer& b) MERAK_JSON_NOEXCEPT { a.swap(b); }

    //@}

    //!@name Append token
    //@{

    //! Append a token and return a new Pointer
    /*!
        \param token Token to be appended.
        \param allocator Allocator for the newly return Pointer.
        \return A new Pointer with appended token.
    */
    GenericPointer Append(const Token& token, Allocator* allocator = 0) const {
        GenericPointer r;
        r.allocator_ = allocator;
        char_type *p = r.CopyFromRaw(*this, 1, token.length + 1);
        std::memcpy(p, token.name, (token.length + 1) * sizeof(char_type));
        r.tokens_[tokenCount_].name = p;
        r.tokens_[tokenCount_].length = token.length;
        r.tokens_[tokenCount_].index = token.index;
        return r;
    }

    //! Append a name token with length, and return a new Pointer
    /*!
        \param name Name to be appended.
        \param length Length of name.
        \param allocator Allocator for the newly return Pointer.
        \return A new Pointer with appended token.
    */
    GenericPointer Append(const char_type* name, SizeType length, Allocator* allocator = 0) const {
        Token token = { name, length, kPointerInvalidIndex };
        return Append(token, allocator);
    }

    //! Append a name token without length, and return a new Pointer
    /*!
        \param name Name (const char_type*) to be appended.
        \param allocator Allocator for the newly return Pointer.
        \return A new Pointer with appended token.
    */
    template <typename T>
    MERAK_JSON_DISABLEIF_RETURN((internal::NotExpr<internal::IsSame<typename internal::RemoveConst<T>::Type, char_type> >), (GenericPointer))
    Append(T* name, Allocator* allocator = 0) const {
        return Append(name, internal::StrLen(name), allocator);
    }


    //! Append a name token, and return a new Pointer
    /*!
        \param name Name to be appended.
        \param allocator Allocator for the newly return Pointer.
        \return A new Pointer with appended token.
    */
    GenericPointer Append(const std::basic_string<char_type>& name, Allocator* allocator = 0) const {
        return Append(name.c_str(), static_cast<SizeType>(name.size()), allocator);
    }


    //! Append a index token, and return a new Pointer
    /*!
        \param index Index to be appended.
        \param allocator Allocator for the newly return Pointer.
        \return A new Pointer with appended token.
    */
    GenericPointer Append(SizeType index, Allocator* allocator = 0) const {
        char buffer[21];
        char* end = sizeof(SizeType) == 4 ? internal::u32toa(index, buffer) : internal::u64toa(index, buffer);
        SizeType length = static_cast<SizeType>(end - buffer);
        buffer[length] = '\0';

        if constexpr (sizeof(char_type) == 1) {
            Token token = { reinterpret_cast<char_type*>(buffer), length, index };
            return Append(token, allocator);
        } else {
            char_type name[21];
            for (size_t i = 0; i <= length; i++)
                name[i] = static_cast<char_type>(buffer[i]);
            Token token = { name, length, index };
            return Append(token, allocator);
        }
    }

    //! Append a token by value, and return a new Pointer
    /*!
        \param token token to be appended.
        \param allocator Allocator for the newly return Pointer.
        \return A new Pointer with appended token.
    */
    GenericPointer Append(const ValueType& token, Allocator* allocator = 0) const {
        if (token.is_string())
            return Append(token.get_string(), token.get_string_length(), allocator);
        else {
            MERAK_JSON_ASSERT(token.is_uint64());
            MERAK_JSON_ASSERT(token.get_uint64() <= SizeType(~0));
            return Append(static_cast<SizeType>(token.get_uint64()), allocator);
        }
    }

    //!@name Handling parse Error
    //@{

    //! Check whether this is a valid pointer.
    bool IsValid() const { return parseErrorCode_ == kPointerParseErrorNone; }

    //! Get the parsing error offset in code unit.
    size_t GetParseErrorOffset() const { return parseErrorOffset_; }

    //! Get the parsing error code.
    PointerParseErrorCode GetParseErrorCode() const { return parseErrorCode_; }

    //@}

    //! Get the allocator of this pointer.
    Allocator& get_allocator() { return *allocator_; }

    //!@name Tokens
    //@{

    //! Get the token array (const version only).
    const Token* GetTokens() const { return tokens_; }

    //! Get the number of tokens.
    size_t GetTokenCount() const { return tokenCount_; }

    //@}

    //!@name Equality/inequality operators
    //@{

    //! Equality operator.
    /*!
        \note When any pointers are invalid, always returns false.
    */
    bool operator==(const GenericPointer& rhs) const {
        if (!IsValid() || !rhs.IsValid() || tokenCount_ != rhs.tokenCount_)
            return false;

        for (size_t i = 0; i < tokenCount_; i++) {
            if (tokens_[i].index != rhs.tokens_[i].index ||
                tokens_[i].length != rhs.tokens_[i].length || 
                (tokens_[i].length != 0 && std::memcmp(tokens_[i].name, rhs.tokens_[i].name, sizeof(char_type)* tokens_[i].length) != 0))
            {
                return false;
            }
        }

        return true;
    }

    //! Inequality operator.
    /*!
        \note When any pointers are invalid, always returns true.
    */
    bool operator!=(const GenericPointer& rhs) const { return !(*this == rhs); }

    //! Less than operator.
    /*!
        \note Invalid pointers are always greater than valid ones.
    */
    bool operator<(const GenericPointer& rhs) const {
        if (!IsValid())
            return false;
        if (!rhs.IsValid())
            return true;

        if (tokenCount_ != rhs.tokenCount_)
            return tokenCount_ < rhs.tokenCount_;

        for (size_t i = 0; i < tokenCount_; i++) {
            if (tokens_[i].index != rhs.tokens_[i].index)
                return tokens_[i].index < rhs.tokens_[i].index;

            if (tokens_[i].length != rhs.tokens_[i].length)
                return tokens_[i].length < rhs.tokens_[i].length;

            if (int cmp = std::memcmp(tokens_[i].name, rhs.tokens_[i].name, sizeof(char_type) * tokens_[i].length))
                return cmp < 0;
        }

        return false;
    }

    //@}

    //!@name Stringify
    //@{

    //! Stringify the pointer into string representation.
    /*!
        \tparam OutputStream Type of output stream.
        \param os The output stream.
    */
    template<typename OutputStream>
    bool Stringify(OutputStream& os) const {
        return Stringify<false, OutputStream>(os);
    }

    //! Stringify the pointer into URI fragment representation.
    /*!
        \tparam OutputStream Type of output stream.
        \param os The output stream.
    */
    template<typename OutputStream>
    bool StringifyUriFragment(OutputStream& os) const {
        return Stringify<true, OutputStream>(os);
    }

    //@}

    //!@name Create value
    //@{

    //! Create a value in a subtree.
    /*!
        If the value is not exist, it creates all parent values and a JSON Null value.
        So it always succeed and return the newly created or existing value.

        Remind that it may change types of parents according to tokens, so it 
        potentially removes previously stored values. For example, if a document 
        was an array, and "/foo" is used to create a value, then the document 
        will be changed to an object, and all existing array elements are lost.

        \param root Root value of a DOM subtree to be resolved. It can be any value other than document root.
        \param allocator Allocator for creating the values if the specified value or its parents are not exist.
        \param alreadyExist If non-null, it stores whether the resolved value is already exist.
        \return The resolved newly created (a JSON Null value), or already exists value.
    */
    ValueType& Create(ValueType& root, typename ValueType::AllocatorType& allocator, bool* alreadyExist = 0) const {
        MERAK_JSON_ASSERT(IsValid());
        ValueType* v = &root;
        bool exist = true;
        for (const Token *t = tokens_; t != tokens_ + tokenCount_; ++t) {
            if (v->is_array() && t->name[0] == '-' && t->length == 1) {
                v->push_back(ValueType().Move(), allocator);
                v = &((*v)[v->size() - 1]);
                exist = false;
            }
            else {
                if (t->index == kPointerInvalidIndex) { // must be object name
                    if (!v->is_object())
                        v->SetObject(); // Change to Object
                }
                else { // object name or array index
                    if (!v->is_array() && !v->is_object())
                        v->set_array(); // Change to Array
                }

                if (v->is_array()) {
                    if (t->index >= v->size()) {
                        v->reserve(t->index + 1, allocator);
                        while (t->index >= v->size())
                            v->push_back(ValueType().Move(), allocator);
                        exist = false;
                    }
                    v = &((*v)[t->index]);
                }
                else {
                    typename ValueType::MemberIterator m = v->find_member(GenericValue<EncodingType>(GenericStringRef<char_type>(t->name, t->length)));
                    if (m == v->member_end()) {
                        v->add_member(ValueType(t->name, t->length, allocator).Move(), ValueType().Move(), allocator);
                        m = v->member_end();
                        v = &(--m)->value; // Assumes add_member() appends at the end
                        exist = false;
                    }
                    else
                        v = &m->value;
                }
            }
        }

        if (alreadyExist)
            *alreadyExist = exist;

        return *v;
    }

    //! Creates a value in a document.
    /*!
        \param document A document to be resolved.
        \param alreadyExist If non-null, it stores whether the resolved value is already exist.
        \return The resolved newly created, or already exists value.
    */
    template <typename stackAllocator>
    ValueType& Create(GenericDocument<EncodingType, typename ValueType::AllocatorType, stackAllocator>& document, bool* alreadyExist = 0) const {
        return Create(document, document.get_allocator(), alreadyExist);
    }

    //@}

    //!@name Compute URI
    //@{

    //! Compute the in-scope URI for a subtree.
    //  For use with JSON pointers into JSON schema documents.
    /*!
        \param root Root value of a DOM sub-tree to be resolved. It can be any value other than document root.
        \param rootUri Root URI
        \param unresolvedTokenIndex If the pointer cannot resolve a token in the pointer, this parameter can obtain the index of unresolved token.
        \param allocator Allocator for Uris
        \return Uri if it can be resolved. Otherwise null.

        \note
        There are only 3 situations when a URI cannot be resolved:
        1. A value in the path is not an array nor object.
        2. An object value does not contain the token.
        3. A token is out of range of an array value.

        Use unresolvedTokenIndex to retrieve the token index.
    */
    UriType GetUri(ValueType& root, const UriType& rootUri, size_t* unresolvedTokenIndex = 0, Allocator* allocator = 0) const {
        static const char_type kIdString[] = { 'i', 'd', '\0' };
        static const ValueType kIdValue(kIdString, 2);
        UriType base = UriType(rootUri, allocator);
        MERAK_JSON_ASSERT(IsValid());
        ValueType* v = &root;
        for (const Token *t = tokens_; t != tokens_ + tokenCount_; ++t) {
            switch (v->get_type()) {
                case kObjectType:
                {
                    // See if we have an id, and if so resolve with the current base
                    typename ValueType::MemberIterator m = v->find_member(kIdValue);
                    if (m != v->member_end() && (m->value).is_string()) {
                        UriType here = UriType(m->value, allocator).Resolve(base, allocator);
                        base = here;
                    }
                    m = v->find_member(GenericValue<EncodingType>(GenericStringRef<char_type>(t->name, t->length)));
                    if (m == v->member_end())
                        break;
                    v = &m->value;
                }
                  continue;
                case kArrayType:
                    if (t->index == kPointerInvalidIndex || t->index >= v->size())
                        break;
                    v = &((*v)[t->index]);
                    continue;
                default:
                    break;
            }

            // Error: unresolved token
            if (unresolvedTokenIndex)
                *unresolvedTokenIndex = static_cast<size_t>(t - tokens_);
            return UriType(allocator);
        }
        return base;
    }

    UriType GetUri(const ValueType& root, const UriType& rootUri, size_t* unresolvedTokenIndex = 0, Allocator* allocator = 0) const {
      return GetUri(const_cast<ValueType&>(root), rootUri, unresolvedTokenIndex, allocator);
    }


    //!@name Query value
    //@{

    //! Query a value in a subtree.
    /*!
        \param root Root value of a DOM sub-tree to be resolved. It can be any value other than document root.
        \param unresolvedTokenIndex If the pointer cannot resolve a token in the pointer, this parameter can obtain the index of unresolved token.
        \return Pointer to the value if it can be resolved. Otherwise null.

        \note
        There are only 3 situations when a value cannot be resolved:
        1. A value in the path is not an array nor object.
        2. An object value does not contain the token.
        3. A token is out of range of an array value.

        Use unresolvedTokenIndex to retrieve the token index.
    */
    ValueType* get(ValueType& root, size_t* unresolvedTokenIndex = 0) const {
        MERAK_JSON_ASSERT(IsValid());
        ValueType* v = &root;
        for (const Token *t = tokens_; t != tokens_ + tokenCount_; ++t) {
            switch (v->get_type()) {
            case kObjectType:
                {
                    typename ValueType::MemberIterator m = v->find_member(GenericValue<EncodingType>(GenericStringRef<char_type>(t->name, t->length)));
                    if (m == v->member_end())
                        break;
                    v = &m->value;
                }
                continue;
            case kArrayType:
                if (t->index == kPointerInvalidIndex || t->index >= v->size())
                    break;
                v = &((*v)[t->index]);
                continue;
            default:
                break;
            }

            // Error: unresolved token
            if (unresolvedTokenIndex)
                *unresolvedTokenIndex = static_cast<size_t>(t - tokens_);
            return 0;
        }
        return v;
    }

    //! Query a const value in a const subtree.
    /*!
        \param root Root value of a DOM sub-tree to be resolved. It can be any value other than document root.
        \return Pointer to the value if it can be resolved. Otherwise null.
    */
    const ValueType* get(const ValueType& root, size_t* unresolvedTokenIndex = 0) const {
        return get(const_cast<ValueType&>(root), unresolvedTokenIndex);
    }

    //@}

    //!@name Query a value with default
    //@{

    //! Query a value in a subtree with default value.
    /*!
        Similar to get(), but if the specified value do not exists, it creates all parents and clone the default value.
        So that this function always succeed.

        \param root Root value of a DOM sub-tree to be resolved. It can be any value other than document root.
        \param defaultValue Default value to be cloned if the value was not exists.
        \param allocator Allocator for creating the values if the specified value or its parents are not exist.
        \see Create()
    */
    ValueType& GetWithDefault(ValueType& root, const ValueType& defaultValue, typename ValueType::AllocatorType& allocator) const {
        bool alreadyExist;
        ValueType& v = Create(root, allocator, &alreadyExist);
        return alreadyExist ? v : v.copy_from(defaultValue, allocator);
    }

    //! Query a value in a subtree with default null-terminated string.
    ValueType& GetWithDefault(ValueType& root, const char_type* defaultValue, typename ValueType::AllocatorType& allocator) const {
        bool alreadyExist;
        ValueType& v = Create(root, allocator, &alreadyExist);
        return alreadyExist ? v : v.set_string(defaultValue, allocator);
    }


    //! Query a value in a subtree with default std::basic_string.
    ValueType& GetWithDefault(ValueType& root, const std::basic_string<char_type>& defaultValue, typename ValueType::AllocatorType& allocator) const {
        bool alreadyExist;
        ValueType& v = Create(root, allocator, &alreadyExist);
        return alreadyExist ? v : v.set_string(defaultValue, allocator);
    }


    //! Query a value in a subtree with default primitive value.
    /*!
        \tparam T Either \ref Type, \c int, \c unsigned, \c int64_t, \c uint64_t, \c bool
    */
    template <typename T>
    MERAK_JSON_DISABLEIF_RETURN((internal::OrExpr<internal::IsPointer<T>, internal::IsGenericValue<T> >), (ValueType&))
    GetWithDefault(ValueType& root, T defaultValue, typename ValueType::AllocatorType& allocator) const {
        return GetWithDefault(root, ValueType(defaultValue).Move(), allocator);
    }

    //! Query a value in a document with default value.
    template <typename stackAllocator>
    ValueType& GetWithDefault(GenericDocument<EncodingType, typename ValueType::AllocatorType, stackAllocator>& document, const ValueType& defaultValue) const {
        return GetWithDefault(document, defaultValue, document.get_allocator());
    }

    //! Query a value in a document with default null-terminated string.
    template <typename stackAllocator>
    ValueType& GetWithDefault(GenericDocument<EncodingType, typename ValueType::AllocatorType, stackAllocator>& document, const char_type* defaultValue) const {
        return GetWithDefault(document, defaultValue, document.get_allocator());
    }


    //! Query a value in a document with default std::basic_string.
    template <typename stackAllocator>
    ValueType& GetWithDefault(GenericDocument<EncodingType, typename ValueType::AllocatorType, stackAllocator>& document, const std::basic_string<char_type>& defaultValue) const {
        return GetWithDefault(document, defaultValue, document.get_allocator());
    }


    //! Query a value in a document with default primitive value.
    /*!
        \tparam T Either \ref Type, \c int, \c unsigned, \c int64_t, \c uint64_t, \c bool
    */
    template <typename T, typename stackAllocator>
    MERAK_JSON_DISABLEIF_RETURN((internal::OrExpr<internal::IsPointer<T>, internal::IsGenericValue<T> >), (ValueType&))
    GetWithDefault(GenericDocument<EncodingType, typename ValueType::AllocatorType, stackAllocator>& document, T defaultValue) const {
        return GetWithDefault(document, defaultValue, document.get_allocator());
    }

    //@}

    //!@name Set a value
    //@{

    //! Set a value in a subtree, with move semantics.
    /*!
        It creates all parents if they are not exist or types are different to the tokens.
        So this function always succeeds but potentially remove existing values.

        \param root Root value of a DOM sub-tree to be resolved. It can be any value other than document root.
        \param value Value to be set.
        \param allocator Allocator for creating the values if the specified value or its parents are not exist.
        \see Create()
    */
    ValueType& set(ValueType& root, ValueType& value, typename ValueType::AllocatorType& allocator) const {
        return Create(root, allocator) = value;
    }

    //! Set a value in a subtree, with copy semantics.
    ValueType& set(ValueType& root, const ValueType& value, typename ValueType::AllocatorType& allocator) const {
        return Create(root, allocator).copy_from(value, allocator);
    }

    //! Set a null-terminated string in a subtree.
    ValueType& set(ValueType& root, const char_type* value, typename ValueType::AllocatorType& allocator) const {
        return Create(root, allocator) = ValueType(value, allocator).Move();
    }


    //! Set a std::basic_string in a subtree.
    ValueType& set(ValueType& root, const std::basic_string<char_type>& value, typename ValueType::AllocatorType& allocator) const {
        return Create(root, allocator) = ValueType(value, allocator).Move();
    }


    //! Set a primitive value in a subtree.
    /*!
        \tparam T Either \ref Type, \c int, \c unsigned, \c int64_t, \c uint64_t, \c bool
    */
    template <typename T>
    MERAK_JSON_DISABLEIF_RETURN((internal::OrExpr<internal::IsPointer<T>, internal::IsGenericValue<T> >), (ValueType&))
    set(ValueType& root, T value, typename ValueType::AllocatorType& allocator) const {
        return Create(root, allocator) = ValueType(value).Move();
    }

    //! Set a value in a document, with move semantics.
    template <typename stackAllocator>
    ValueType& set(GenericDocument<EncodingType, typename ValueType::AllocatorType, stackAllocator>& document, ValueType& value) const {
        return Create(document) = value;
    }

    //! Set a value in a document, with copy semantics.
    template <typename stackAllocator>
    ValueType& set(GenericDocument<EncodingType, typename ValueType::AllocatorType, stackAllocator>& document, const ValueType& value) const {
        return Create(document).copy_from(value, document.get_allocator());
    }

    //! Set a null-terminated string in a document.
    template <typename stackAllocator>
    ValueType& set(GenericDocument<EncodingType, typename ValueType::AllocatorType, stackAllocator>& document, const char_type* value) const {
        return Create(document) = ValueType(value, document.get_allocator()).Move();
    }


    //! Sets a std::basic_string in a document.
    template <typename stackAllocator>
    ValueType& set(GenericDocument<EncodingType, typename ValueType::AllocatorType, stackAllocator>& document, const std::basic_string<char_type>& value) const {
        return Create(document) = ValueType(value, document.get_allocator()).Move();
    }


    //! Set a primitive value in a document.
    /*!
    \tparam T Either \ref Type, \c int, \c unsigned, \c int64_t, \c uint64_t, \c bool
    */
    template <typename T, typename stackAllocator>
    MERAK_JSON_DISABLEIF_RETURN((internal::OrExpr<internal::IsPointer<T>, internal::IsGenericValue<T> >), (ValueType&))
    set(GenericDocument<EncodingType, typename ValueType::AllocatorType, stackAllocator>& document, T value) const {
            return Create(document) = value;
    }

    //@}

    //!@name swap a value
    //@{

    //! swap a value with a value in a subtree.
    /*!
        It creates all parents if they are not exist or types are different to the tokens.
        So this function always succeeds but potentially remove existing values.

        \param root Root value of a DOM sub-tree to be resolved. It can be any value other than document root.
        \param value Value to be swapped.
        \param allocator Allocator for creating the values if the specified value or its parents are not exist.
        \see Create()
    */
    ValueType& swap(ValueType& root, ValueType& value, typename ValueType::AllocatorType& allocator) const {
        return Create(root, allocator).swap(value);
    }

    //! swap a value with a value in a document.
    template <typename stackAllocator>
    ValueType& swap(GenericDocument<EncodingType, typename ValueType::AllocatorType, stackAllocator>& document, ValueType& value) const {
        return Create(document).swap(value);
    }

    //@}

    //! erase a value in a subtree.
    /*!
        \param root Root value of a DOM sub-tree to be resolved. It can be any value other than document root.
        \return Whether the resolved value is found and erased.

        \note Erasing with an empty pointer \c Pointer(""), i.e. the root, always fail and return false.
    */
    bool erase(ValueType& root) const {
        MERAK_JSON_ASSERT(IsValid());
        if (tokenCount_ == 0) // Cannot erase the root
            return false;

        ValueType* v = &root;
        const Token* last = tokens_ + (tokenCount_ - 1);
        for (const Token *t = tokens_; t != last; ++t) {
            switch (v->get_type()) {
            case kObjectType:
                {
                    typename ValueType::MemberIterator m = v->find_member(GenericValue<EncodingType>(GenericStringRef<char_type>(t->name, t->length)));
                    if (m == v->member_end())
                        return false;
                    v = &m->value;
                }
                break;
            case kArrayType:
                if (t->index == kPointerInvalidIndex || t->index >= v->size())
                    return false;
                v = &((*v)[t->index]);
                break;
            default:
                return false;
            }
        }

        switch (v->get_type()) {
        case kObjectType:
            return v->erase_member(GenericStringRef<char_type>(last->name, last->length));
        case kArrayType:
            if (last->index == kPointerInvalidIndex || last->index >= v->size())
                return false;
            v->erase(v->begin() + last->index);
            return true;
        default:
            return false;
        }
    }

private:
    //! Clone the content from rhs to this.
    /*!
        \param rhs Source pointer.
        \param extraToken Extra tokens to be allocated.
        \param extraNameBufferSize Extra name buffer size (in number of char_type) to be allocated.
        \return Start of non-occupied name buffer, for storing extra names.
    */
    char_type* CopyFromRaw(const GenericPointer& rhs, size_t extraToken = 0, size_t extraNameBufferSize = 0) {
        if (!allocator_) // allocator is independently owned.
            ownAllocator_ = allocator_ = MERAK_JSON_NEW(Allocator)();

        size_t nameBufferSize = rhs.tokenCount_; // null terminators for tokens
        for (Token *t = rhs.tokens_; t != rhs.tokens_ + rhs.tokenCount_; ++t)
            nameBufferSize += t->length;

        tokenCount_ = rhs.tokenCount_ + extraToken;
        tokens_ = static_cast<Token *>(allocator_->Malloc(tokenCount_ * sizeof(Token) + (nameBufferSize + extraNameBufferSize) * sizeof(char_type)));
        nameBuffer_ = reinterpret_cast<char_type *>(tokens_ + tokenCount_);
        if (rhs.tokenCount_ > 0) {
            std::memcpy(tokens_, rhs.tokens_, rhs.tokenCount_ * sizeof(Token));
        }
        if (nameBufferSize > 0) {
            std::memcpy(nameBuffer_, rhs.nameBuffer_, nameBufferSize * sizeof(char_type));
        }

        // The names of each token point to a string in the nameBuffer_. The
        // previous memcpy copied over string pointers into the rhs.nameBuffer_,
        // but they should point to the strings in the new nameBuffer_.
        for (size_t i = 0; i < rhs.tokenCount_; ++i) {
          // The offset between the string address and the name buffer should
          // still be constant, so we can just get this offset and set each new
          // token name according the new buffer start + the known offset.
          std::ptrdiff_t name_offset = rhs.tokens_[i].name - rhs.nameBuffer_;
          tokens_[i].name = nameBuffer_ + name_offset;
        }

        return nameBuffer_ + nameBufferSize;
    }

    //! Check whether a character should be percent-encoded.
    /*!
        According to RFC 3986 2.3 Unreserved Characters.
        \param c The character (code unit) to be tested.
    */
    bool NeedPercentEncode(char_type c) const {
        return !((c >= '0' && c <= '9') || (c >= 'A' && c <='Z') || (c >= 'a' && c <= 'z') || c == '-' || c == '.' || c == '_' || c =='~');
    }

    //! parse a JSON String or its URI fragment representation into tokens.
#ifndef __clang__ // -Wdocumentation
    /*!
        \param source Either a JSON Pointer string, or its URI fragment representation. Not need to be null terminated.
        \param length Length of the source string.
        \note Source cannot be JSON String Representation of JSON Pointer, e.g. In "/\u0000", \u0000 will not be unescaped.
    */
#endif
    void parse(const char_type* source, size_t length) {
        MERAK_JSON_ASSERT(source != NULL);
        MERAK_JSON_ASSERT(nameBuffer_ == 0);
        MERAK_JSON_ASSERT(tokens_ == 0);

        // Create own allocator if user did not supply.
        if (!allocator_)
            ownAllocator_ = allocator_ = MERAK_JSON_NEW(Allocator)();

        // Count number of '/' as tokenCount
        tokenCount_ = 0;
        for (const char_type* s = source; s != source + length; s++)
            if (*s == '/')
                tokenCount_++;

        Token* token = tokens_ = static_cast<Token *>(allocator_->Malloc(tokenCount_ * sizeof(Token) + length * sizeof(char_type)));
        char_type* name = nameBuffer_ = reinterpret_cast<char_type *>(tokens_ + tokenCount_);
        size_t i = 0;

        // Detect if it is a URI fragment
        bool uriFragment = false;
        if (source[i] == '#') {
            uriFragment = true;
            i++;
        }

        if (i != length && source[i] != '/') {
            parseErrorCode_ = kPointerParseErrorTokenMustBeginWithSolidus;
            goto error;
        }

        while (i < length) {
            MERAK_JSON_ASSERT(source[i] == '/');
            i++; // consumes '/'

            token->name = name;
            bool isNumber = true;

            while (i < length && source[i] != '/') {
                char_type c = source[i];
                if (uriFragment) {
                    // Decoding percent-encoding for URI fragment
                    if (c == '%') {
                        PercentDecodeStream is(&source[i], source + length);
                        GenericInsituStringStream<EncodingType> os(name);
                        char_type* begin = os.PutBegin();
                        if (!Transcoder<UTF8<>, EncodingType>().Validate(is, os) || !is.IsValid()) {
                            parseErrorCode_ = kPointerParseErrorInvalidPercentEncoding;
                            goto error;
                        }
                        size_t len = os.PutEnd(begin);
                        i += is.tellg() - 1;
                        if (len == 1)
                            c = *name;
                        else {
                            name += len;
                            isNumber = false;
                            i++;
                            continue;
                        }
                    }
                    else if (NeedPercentEncode(c)) {
                        parseErrorCode_ = kPointerParseErrorCharacterMustPercentEncode;
                        goto error;
                    }
                }

                i++;

                // Escaping "~0" -> '~', "~1" -> '/'
                if (c == '~') {
                    if (i < length) {
                        c = source[i];
                        if (c == '0')       c = '~';
                        else if (c == '1')  c = '/';
                        else {
                            parseErrorCode_ = kPointerParseErrorInvalidEscape;
                            goto error;
                        }
                        i++;
                    }
                    else {
                        parseErrorCode_ = kPointerParseErrorInvalidEscape;
                        goto error;
                    }
                }

                // First check for index: all of characters are digit
                if (c < '0' || c > '9')
                    isNumber = false;

                *name++ = c;
            }
            token->length = static_cast<SizeType>(name - token->name);
            if (token->length == 0)
                isNumber = false;
            *name++ = '\0'; // Null terminator

            // Second check for index: more than one digit cannot have leading zero
            if (isNumber && token->length > 1 && token->name[0] == '0')
                isNumber = false;

            // String to SizeType conversion
            SizeType n = 0;
            if (isNumber) {
                for (size_t j = 0; j < token->length; j++) {
                    SizeType m = n * 10 + static_cast<SizeType>(token->name[j] - '0');
                    if (m < n) {   // overflow detection
                        isNumber = false;
                        break;
                    }
                    n = m;
                }
            }

            token->index = isNumber ? n : kPointerInvalidIndex;
            token++;
        }

        MERAK_JSON_ASSERT(name <= nameBuffer_ + length); // Should not overflow buffer
        parseErrorCode_ = kPointerParseErrorNone;
        return;

    error:
        Allocator::Free(tokens_);
        nameBuffer_ = 0;
        tokens_ = 0;
        tokenCount_ = 0;
        parseErrorOffset_ = i;
        return;
    }

    //! Stringify to string or URI fragment representation.
    /*!
        \tparam uriFragment True for stringifying to URI fragment representation. False for string representation.
        \tparam OutputStream type of output stream.
        \param os The output stream.
    */
    template<bool uriFragment, typename OutputStream>
    bool Stringify(OutputStream& os) const {
        MERAK_JSON_ASSERT(IsValid());

        if (uriFragment)
            os.put('#');

        for (Token *t = tokens_; t != tokens_ + tokenCount_; ++t) {
            os.put('/');
            for (size_t j = 0; j < t->length; j++) {
                char_type c = t->name[j];
                if (c == '~') {
                    os.put('~');
                    os.put('0');
                }
                else if (c == '/') {
                    os.put('~');
                    os.put('1');
                }
                else if (uriFragment && NeedPercentEncode(c)) { 
                    // Transcode to UTF8 sequence
                    GenericStringStream<typename ValueType::EncodingType> source(&t->name[j]);
                    PercentEncodeStream<OutputStream> target(os);
                    if (!Transcoder<EncodingType, UTF8<> >().Validate(source, target))
                        return false;
                    j += source.tellg() - 1;
                }
                else
                    os.put(c);
            }
        }
        return true;
    }

    //! A helper stream for decoding a percent-encoded sequence into code unit.
    /*!
        This stream decodes %XY triplet into code unit (0-255).
        If it encounters invalid characters, it sets output code unit as 0 and 
        mark invalid, and to be checked by IsValid().
    */
    class PercentDecodeStream {
    public:
        typedef typename ValueType::char_type char_type;

        //! Constructor
        /*!
            \param source Start of the stream
            \param end Past-the-end of the stream.
        */
        PercentDecodeStream(const char_type* source, const char_type* end) : src_(source), head_(source), end_(end), valid_(true) {}

        char_type get() {
            if (*src_ != '%' || src_ + 3 > end_) { // %XY triplet
                valid_ = false;
                return 0;
            }
            src_++;
            char_type c = 0;
            for (int j = 0; j < 2; j++) {
                c = static_cast<char_type>(c << 4);
                char_type h = *src_;
                if      (h >= '0' && h <= '9') c = static_cast<char_type>(c + h - '0');
                else if (h >= 'A' && h <= 'F') c = static_cast<char_type>(c + h - 'A' + 10);
                else if (h >= 'a' && h <= 'f') c = static_cast<char_type>(c + h - 'a' + 10);
                else {
                    valid_ = false;
                    return 0;
                }
                src_++;
            }
            return c;
        }

        size_t tellg() const { return static_cast<size_t>(src_ - head_); }
        bool IsValid() const { return valid_; }

    private:
        const char_type* src_;     //!< Current read position.
        const char_type* head_;    //!< Original head of the string.
        const char_type* end_;     //!< Past-the-end position.
        bool valid_;        //!< Whether the parsing is valid.
    };

    //! A helper stream to encode character (UTF-8 code unit) into percent-encoded sequence.
    template <typename OutputStream>
    class PercentEncodeStream {
    public:
        PercentEncodeStream(OutputStream& os) : os_(os) {}
        void put(char c) { // UTF-8 must be byte
            unsigned char u = static_cast<unsigned char>(c);
            static const char hexDigits[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
            os_.put('%');
            os_.put(static_cast<typename OutputStream::char_type>(hexDigits[u >> 4]));
            os_.put(static_cast<typename OutputStream::char_type>(hexDigits[u & 15]));
        }
    private:
        OutputStream& os_;
    };

    Allocator* allocator_;                  //!< The current allocator. It is either user-supplied or equal to ownAllocator_.
    Allocator* ownAllocator_;               //!< Allocator owned by this Pointer.
    char_type* nameBuffer_;                        //!< A buffer containing all names in tokens.
    Token* tokens_;                         //!< A list of tokens.
    size_t tokenCount_;                     //!< Number of tokens in tokens_.
    size_t parseErrorOffset_;               //!< Offset in code unit when parsing fail.
    PointerParseErrorCode parseErrorCode_;  //!< Parsing error code.
};

//! GenericPointer for Value (UTF-8, default allocator).
typedef GenericPointer<Value> Pointer;

//!@name Helper functions for GenericPointer
//@{

//////////////////////////////////////////////////////////////////////////////

template <typename T>
typename T::ValueType& CreateValueByPointer(T& root, const GenericPointer<typename T::ValueType>& pointer, typename T::AllocatorType& a) {
    return pointer.Create(root, a);
}

template <typename T, typename CharType, size_t N>
typename T::ValueType& CreateValueByPointer(T& root, const CharType(&source)[N], typename T::AllocatorType& a) {
    return GenericPointer<typename T::ValueType>(source, N - 1).Create(root, a);
}

// No allocator parameter

template <typename DocumentType>
typename DocumentType::ValueType& CreateValueByPointer(DocumentType& document, const GenericPointer<typename DocumentType::ValueType>& pointer) {
    return pointer.Create(document);
}

template <typename DocumentType, typename CharType, size_t N>
typename DocumentType::ValueType& CreateValueByPointer(DocumentType& document, const CharType(&source)[N]) {
    return GenericPointer<typename DocumentType::ValueType>(source, N - 1).Create(document);
}

//////////////////////////////////////////////////////////////////////////////

template <typename T>
typename T::ValueType* GetValueByPointer(T& root, const GenericPointer<typename T::ValueType>& pointer, size_t* unresolvedTokenIndex = 0) {
    return pointer.get(root, unresolvedTokenIndex);
}

template <typename T>
const typename T::ValueType* GetValueByPointer(const T& root, const GenericPointer<typename T::ValueType>& pointer, size_t* unresolvedTokenIndex = 0) {
    return pointer.get(root, unresolvedTokenIndex);
}

template <typename T, typename CharType, size_t N>
typename T::ValueType* GetValueByPointer(T& root, const CharType (&source)[N], size_t* unresolvedTokenIndex = 0) {
    return GenericPointer<typename T::ValueType>(source, N - 1).get(root, unresolvedTokenIndex);
}

template <typename T, typename CharType, size_t N>
const typename T::ValueType* GetValueByPointer(const T& root, const CharType(&source)[N], size_t* unresolvedTokenIndex = 0) {
    return GenericPointer<typename T::ValueType>(source, N - 1).get(root, unresolvedTokenIndex);
}

//////////////////////////////////////////////////////////////////////////////

template <typename T>
typename T::ValueType& GetValueByPointerWithDefault(T& root, const GenericPointer<typename T::ValueType>& pointer, const typename T::ValueType& defaultValue, typename T::AllocatorType& a) {
    return pointer.GetWithDefault(root, defaultValue, a);
}

template <typename T>
typename T::ValueType& GetValueByPointerWithDefault(T& root, const GenericPointer<typename T::ValueType>& pointer, const typename T::char_type* defaultValue, typename T::AllocatorType& a) {
    return pointer.GetWithDefault(root, defaultValue, a);
}


template <typename T>
typename T::ValueType& GetValueByPointerWithDefault(T& root, const GenericPointer<typename T::ValueType>& pointer, const std::basic_string<typename T::char_type>& defaultValue, typename T::AllocatorType& a) {
    return pointer.GetWithDefault(root, defaultValue, a);
}


template <typename T, typename T2>
MERAK_JSON_DISABLEIF_RETURN((internal::OrExpr<internal::IsPointer<T2>, internal::IsGenericValue<T2> >), (typename T::ValueType&))
GetValueByPointerWithDefault(T& root, const GenericPointer<typename T::ValueType>& pointer, T2 defaultValue, typename T::AllocatorType& a) {
    return pointer.GetWithDefault(root, defaultValue, a);
}

template <typename T, typename CharType, size_t N>
typename T::ValueType& GetValueByPointerWithDefault(T& root, const CharType(&source)[N], const typename T::ValueType& defaultValue, typename T::AllocatorType& a) {
    return GenericPointer<typename T::ValueType>(source, N - 1).GetWithDefault(root, defaultValue, a);
}

template <typename T, typename CharType, size_t N>
typename T::ValueType& GetValueByPointerWithDefault(T& root, const CharType(&source)[N], const typename T::char_type* defaultValue, typename T::AllocatorType& a) {
    return GenericPointer<typename T::ValueType>(source, N - 1).GetWithDefault(root, defaultValue, a);
}


template <typename T, typename CharType, size_t N>
typename T::ValueType& GetValueByPointerWithDefault(T& root, const CharType(&source)[N], const std::basic_string<typename T::char_type>& defaultValue, typename T::AllocatorType& a) {
    return GenericPointer<typename T::ValueType>(source, N - 1).GetWithDefault(root, defaultValue, a);
}


template <typename T, typename CharType, size_t N, typename T2>
MERAK_JSON_DISABLEIF_RETURN((internal::OrExpr<internal::IsPointer<T2>, internal::IsGenericValue<T2> >), (typename T::ValueType&))
GetValueByPointerWithDefault(T& root, const CharType(&source)[N], T2 defaultValue, typename T::AllocatorType& a) {
    return GenericPointer<typename T::ValueType>(source, N - 1).GetWithDefault(root, defaultValue, a);
}

// No allocator parameter

template <typename DocumentType>
typename DocumentType::ValueType& GetValueByPointerWithDefault(DocumentType& document, const GenericPointer<typename DocumentType::ValueType>& pointer, const typename DocumentType::ValueType& defaultValue) {
    return pointer.GetWithDefault(document, defaultValue);
}

template <typename DocumentType>
typename DocumentType::ValueType& GetValueByPointerWithDefault(DocumentType& document, const GenericPointer<typename DocumentType::ValueType>& pointer, const typename DocumentType::char_type* defaultValue) {
    return pointer.GetWithDefault(document, defaultValue);
}


template <typename DocumentType>
typename DocumentType::ValueType& GetValueByPointerWithDefault(DocumentType& document, const GenericPointer<typename DocumentType::ValueType>& pointer, const std::basic_string<typename DocumentType::char_type>& defaultValue) {
    return pointer.GetWithDefault(document, defaultValue);
}


template <typename DocumentType, typename T2>
MERAK_JSON_DISABLEIF_RETURN((internal::OrExpr<internal::IsPointer<T2>, internal::IsGenericValue<T2> >), (typename DocumentType::ValueType&))
GetValueByPointerWithDefault(DocumentType& document, const GenericPointer<typename DocumentType::ValueType>& pointer, T2 defaultValue) {
    return pointer.GetWithDefault(document, defaultValue);
}

template <typename DocumentType, typename CharType, size_t N>
typename DocumentType::ValueType& GetValueByPointerWithDefault(DocumentType& document, const CharType(&source)[N], const typename DocumentType::ValueType& defaultValue) {
    return GenericPointer<typename DocumentType::ValueType>(source, N - 1).GetWithDefault(document, defaultValue);
}

template <typename DocumentType, typename CharType, size_t N>
typename DocumentType::ValueType& GetValueByPointerWithDefault(DocumentType& document, const CharType(&source)[N], const typename DocumentType::char_type* defaultValue) {
    return GenericPointer<typename DocumentType::ValueType>(source, N - 1).GetWithDefault(document, defaultValue);
}


template <typename DocumentType, typename CharType, size_t N>
typename DocumentType::ValueType& GetValueByPointerWithDefault(DocumentType& document, const CharType(&source)[N], const std::basic_string<typename DocumentType::char_type>& defaultValue) {
    return GenericPointer<typename DocumentType::ValueType>(source, N - 1).GetWithDefault(document, defaultValue);
}


template <typename DocumentType, typename CharType, size_t N, typename T2>
MERAK_JSON_DISABLEIF_RETURN((internal::OrExpr<internal::IsPointer<T2>, internal::IsGenericValue<T2> >), (typename DocumentType::ValueType&))
GetValueByPointerWithDefault(DocumentType& document, const CharType(&source)[N], T2 defaultValue) {
    return GenericPointer<typename DocumentType::ValueType>(source, N - 1).GetWithDefault(document, defaultValue);
}

//////////////////////////////////////////////////////////////////////////////

template <typename T>
typename T::ValueType& SetValueByPointer(T& root, const GenericPointer<typename T::ValueType>& pointer, typename T::ValueType& value, typename T::AllocatorType& a) {
    return pointer.set(root, value, a);
}

template <typename T>
typename T::ValueType& SetValueByPointer(T& root, const GenericPointer<typename T::ValueType>& pointer, const typename T::ValueType& value, typename T::AllocatorType& a) {
    return pointer.set(root, value, a);
}

template <typename T>
typename T::ValueType& SetValueByPointer(T& root, const GenericPointer<typename T::ValueType>& pointer, const typename T::char_type* value, typename T::AllocatorType& a) {
    return pointer.set(root, value, a);
}


template <typename T>
typename T::ValueType& SetValueByPointer(T& root, const GenericPointer<typename T::ValueType>& pointer, const std::basic_string<typename T::char_type>& value, typename T::AllocatorType& a) {
    return pointer.set(root, value, a);
}


template <typename T, typename T2>
MERAK_JSON_DISABLEIF_RETURN((internal::OrExpr<internal::IsPointer<T2>, internal::IsGenericValue<T2> >), (typename T::ValueType&))
SetValueByPointer(T& root, const GenericPointer<typename T::ValueType>& pointer, T2 value, typename T::AllocatorType& a) {
    return pointer.set(root, value, a);
}

template <typename T, typename CharType, size_t N>
typename T::ValueType& SetValueByPointer(T& root, const CharType(&source)[N], typename T::ValueType& value, typename T::AllocatorType& a) {
    return GenericPointer<typename T::ValueType>(source, N - 1).set(root, value, a);
}

template <typename T, typename CharType, size_t N>
typename T::ValueType& SetValueByPointer(T& root, const CharType(&source)[N], const typename T::ValueType& value, typename T::AllocatorType& a) {
    return GenericPointer<typename T::ValueType>(source, N - 1).set(root, value, a);
}

template <typename T, typename CharType, size_t N>
typename T::ValueType& SetValueByPointer(T& root, const CharType(&source)[N], const typename T::char_type* value, typename T::AllocatorType& a) {
    return GenericPointer<typename T::ValueType>(source, N - 1).set(root, value, a);
}


template <typename T, typename CharType, size_t N>
typename T::ValueType& SetValueByPointer(T& root, const CharType(&source)[N], const std::basic_string<typename T::char_type>& value, typename T::AllocatorType& a) {
    return GenericPointer<typename T::ValueType>(source, N - 1).set(root, value, a);
}


template <typename T, typename CharType, size_t N, typename T2>
MERAK_JSON_DISABLEIF_RETURN((internal::OrExpr<internal::IsPointer<T2>, internal::IsGenericValue<T2> >), (typename T::ValueType&))
SetValueByPointer(T& root, const CharType(&source)[N], T2 value, typename T::AllocatorType& a) {
    return GenericPointer<typename T::ValueType>(source, N - 1).set(root, value, a);
}

// No allocator parameter

template <typename DocumentType>
typename DocumentType::ValueType& SetValueByPointer(DocumentType& document, const GenericPointer<typename DocumentType::ValueType>& pointer, typename DocumentType::ValueType& value) {
    return pointer.set(document, value);
}

template <typename DocumentType>
typename DocumentType::ValueType& SetValueByPointer(DocumentType& document, const GenericPointer<typename DocumentType::ValueType>& pointer, const typename DocumentType::ValueType& value) {
    return pointer.set(document, value);
}

template <typename DocumentType>
typename DocumentType::ValueType& SetValueByPointer(DocumentType& document, const GenericPointer<typename DocumentType::ValueType>& pointer, const typename DocumentType::char_type* value) {
    return pointer.set(document, value);
}


template <typename DocumentType>
typename DocumentType::ValueType& SetValueByPointer(DocumentType& document, const GenericPointer<typename DocumentType::ValueType>& pointer, const std::basic_string<typename DocumentType::char_type>& value) {
    return pointer.set(document, value);
}


template <typename DocumentType, typename T2>
MERAK_JSON_DISABLEIF_RETURN((internal::OrExpr<internal::IsPointer<T2>, internal::IsGenericValue<T2> >), (typename DocumentType::ValueType&))
SetValueByPointer(DocumentType& document, const GenericPointer<typename DocumentType::ValueType>& pointer, T2 value) {
    return pointer.set(document, value);
}

template <typename DocumentType, typename CharType, size_t N>
typename DocumentType::ValueType& SetValueByPointer(DocumentType& document, const CharType(&source)[N], typename DocumentType::ValueType& value) {
    return GenericPointer<typename DocumentType::ValueType>(source, N - 1).set(document, value);
}

template <typename DocumentType, typename CharType, size_t N>
typename DocumentType::ValueType& SetValueByPointer(DocumentType& document, const CharType(&source)[N], const typename DocumentType::ValueType& value) {
    return GenericPointer<typename DocumentType::ValueType>(source, N - 1).set(document, value);
}

template <typename DocumentType, typename CharType, size_t N>
typename DocumentType::ValueType& SetValueByPointer(DocumentType& document, const CharType(&source)[N], const typename DocumentType::char_type* value) {
    return GenericPointer<typename DocumentType::ValueType>(source, N - 1).set(document, value);
}


template <typename DocumentType, typename CharType, size_t N>
typename DocumentType::ValueType& SetValueByPointer(DocumentType& document, const CharType(&source)[N], const std::basic_string<typename DocumentType::char_type>& value) {
    return GenericPointer<typename DocumentType::ValueType>(source, N - 1).set(document, value);
}


template <typename DocumentType, typename CharType, size_t N, typename T2>
MERAK_JSON_DISABLEIF_RETURN((internal::OrExpr<internal::IsPointer<T2>, internal::IsGenericValue<T2> >), (typename DocumentType::ValueType&))
SetValueByPointer(DocumentType& document, const CharType(&source)[N], T2 value) {
    return GenericPointer<typename DocumentType::ValueType>(source, N - 1).set(document, value);
}

//////////////////////////////////////////////////////////////////////////////

template <typename T>
typename T::ValueType& SwapValueByPointer(T& root, const GenericPointer<typename T::ValueType>& pointer, typename T::ValueType& value, typename T::AllocatorType& a) {
    return pointer.swap(root, value, a);
}

template <typename T, typename CharType, size_t N>
typename T::ValueType& SwapValueByPointer(T& root, const CharType(&source)[N], typename T::ValueType& value, typename T::AllocatorType& a) {
    return GenericPointer<typename T::ValueType>(source, N - 1).swap(root, value, a);
}

template <typename DocumentType>
typename DocumentType::ValueType& SwapValueByPointer(DocumentType& document, const GenericPointer<typename DocumentType::ValueType>& pointer, typename DocumentType::ValueType& value) {
    return pointer.swap(document, value);
}

template <typename DocumentType, typename CharType, size_t N>
typename DocumentType::ValueType& SwapValueByPointer(DocumentType& document, const CharType(&source)[N], typename DocumentType::ValueType& value) {
    return GenericPointer<typename DocumentType::ValueType>(source, N - 1).swap(document, value);
}

//////////////////////////////////////////////////////////////////////////////

template <typename T>
bool EraseValueByPointer(T& root, const GenericPointer<typename T::ValueType>& pointer) {
    return pointer.erase(root);
}

template <typename T, typename CharType, size_t N>
bool EraseValueByPointer(T& root, const CharType(&source)[N]) {
    return GenericPointer<typename T::ValueType>(source, N - 1).erase(root);
}

//@}

}  // namespace merak::json

#if defined(__clang__) || defined(_MSC_VER)
MERAK_JSON_DIAG_POP
#endif

#endif // MERAK_JSON_POINTER_H_
