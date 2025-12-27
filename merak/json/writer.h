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

#ifndef MERAK_JSON_WRITER_H_
#define MERAK_JSON_WRITER_H_

#include <merak/json/stream.h>
#include <merak/json/internal/clzll.h>
#include <merak/json/internal/meta.h>
#include <merak/json/internal/stack.h>
#include <merak/json/internal/strfunc.h>
#include <merak/json/internal/dtoa.h>
#include <merak/json/internal/itoa.h>
#include <merak/json/stringbuffer.h>
#include <new>      // placement new

#if defined(MERAK_JSON_SIMD) && defined(_MSC_VER)
#include <intrin.h>
#pragma intrinsic(_BitScanForward)
#endif
#ifdef MERAK_JSON_SSE42
#include <nmmintrin.h>
#elif defined(MERAK_JSON_SSE2)
#include <emmintrin.h>
#elif defined(MERAK_JSON_NEON)
#include <arm_neon.h>
#endif

#ifdef __clang__
MERAK_JSON_DIAG_PUSH
MERAK_JSON_DIAG_OFF(padded)
MERAK_JSON_DIAG_OFF(unreachable-code)
MERAK_JSON_DIAG_OFF(c++98-compat)
#elif defined(_MSC_VER)
MERAK_JSON_DIAG_PUSH
MERAK_JSON_DIAG_OFF(4127) // conditional expression is constant
#endif

namespace merak::json {

///////////////////////////////////////////////////////////////////////////////
// WriteFlag

/*! \def MERAK_JSON_WRITE_DEFAULT_FLAGS
    \ingroup MERAK_JSON_CONFIG
    \brief User-defined kWriteDefaultFlags definition.

    User can define this as any \c WriteFlag combinations.
*/
#ifndef MERAK_JSON_WRITE_DEFAULT_FLAGS
#define MERAK_JSON_WRITE_DEFAULT_FLAGS kWriteNoFlags
#endif

//! Combination of writeFlags
enum WriteFlag {
    kWriteNoFlags = 0,              //!< No flags are set.
    kWriteValidateEncodingFlag = 1, //!< Validate encoding of JSON strings.
    kWriteNanAndInfFlag = 2,        //!< Allow writing of Infinity, -Infinity and NaN.
    kWriteNanAndInfNullFlag = 4,    //!< Allow writing of Infinity, -Infinity and NaN as null.
    kWriteDefaultFlags = MERAK_JSON_WRITE_DEFAULT_FLAGS  //!< Default write flags. Can be customized by defining MERAK_JSON_WRITE_DEFAULT_FLAGS
};

    template<typename T, typename = void>
    struct HasPuts : std::false_type {
    };

    template<typename T>
    struct HasPuts<
            T, std::enable_if_t<std::is_same<decltype(std::declval<T>().write(
                    std::declval<const typename T::char_type*>(), std::declval<size_t>())), void>::value>>
            : std::true_type {
    };

//! JSON writer
/*! Writer implements the concept Handler.
    It generates JSON text by events to an output os.

    User may programmatically calls the functions of a writer to generate JSON text.

    On the other side, a writer can also be passed to objects that generates events, 

    for example Reader::parse() and Document::accept().

    \tparam OutputStream Type of output stream.
    \tparam SourceEncoding Encoding of source string.
    \tparam TargetEncoding Encoding of output stream.
    \tparam StackAllocator Type of allocator for allocating memory of stack.
    \note implements Handler concept
*/
template<typename OutputStream, typename SourceEncoding = UTF8<>, typename TargetEncoding = UTF8<>, typename StackAllocator = CrtAllocator, unsigned writeFlags = kWriteDefaultFlags>
class Writer {
public:
    typedef typename SourceEncoding::char_type char_type;

    static const int kDefaultMaxDecimalPlaces = 324;

    //! Constructor
    /*! \param os Output stream.
        \param stackAllocator User supplied allocator. If it is null, it will create a private one.
        \param levelDepth Initial capacity of stack.
    */
    explicit
    Writer(OutputStream& os, StackAllocator* stackAllocator = 0, size_t levelDepth = kDefaultLevelDepth) : 
        os_(&os), level_stack_(stackAllocator, levelDepth * sizeof(Level)), maxDecimalPlaces_(kDefaultMaxDecimalPlaces), hasRoot_(false) {}

    explicit
    Writer(StackAllocator* allocator = 0, size_t levelDepth = kDefaultLevelDepth) :
        os_(0), level_stack_(allocator, levelDepth * sizeof(Level)), maxDecimalPlaces_(kDefaultMaxDecimalPlaces), hasRoot_(false) {}

    Writer(Writer&& rhs) :
        os_(rhs.os_), level_stack_(std::move(rhs.level_stack_)), maxDecimalPlaces_(rhs.maxDecimalPlaces_), hasRoot_(rhs.hasRoot_) {
        rhs.os_ = 0;
    }

    //! Reset the writer with a new stream.
    /*!
        This function reset the writer with a new stream and default settings,
        in order to make a Writer object reusable for output multiple JSONs.

        \param os New output stream.
        \code
        Writer<OutputStream> writer(os1);
        writer.start_object();
        // ...
        writer.end_object();

        writer.Reset(os2);
        writer.start_object();
        // ...
        writer.end_object();
        \endcode
    */
    void Reset(OutputStream& os) {
        os_ = &os;
        hasRoot_ = false;
        level_stack_.clear();
    }

    //! Checks whether the output is a complete JSON.
    /*!
        A complete JSON has a complete root object or array.
    */
    [[nodiscard]] bool is_complete() const {
        return hasRoot_ && level_stack_.empty();
    }

    [[nodiscard]] int get_max_decimal_places() const {
        return maxDecimalPlaces_;
    }

    //! Sets the maximum number of decimal places for double output.
    /*!
        This setting truncates the output with specified number of decimal places.

        For example, 

        \code
        writer.set_max_decimal_places(3);
        writer.start_array();
        writer.emplace_double(0.12345);                 // "0.123"
        writer.emplace_double(0.0001);                  // "0.0"
        writer.emplace_double(1.234567890123456e30);    // "1.234567890123456e30" (do not truncate significand for positive exponent)
        writer.emplace_double(1.23e-4);                 // "0.0"                  (do truncate significand for negative exponent)
        writer.end_array();
        \endcode

        The default setting does not truncate any decimal places. You can restore to this setting by calling
        \code
        writer.set_max_decimal_places(Writer::kDefaultMaxDecimalPlaces);
        \endcode
    */
    void set_max_decimal_places(int maxDecimalPlaces) {
        maxDecimalPlaces_ = maxDecimalPlaces;
    }

    /*!@name Implementation of Handler
        \see Handler
    */
    //@{

    bool emplace_null()                 { Prefix(kNullType);   return EndValue(WriteNull()); }
    bool emplace_bool(bool b)           { Prefix(b ? kTrueType : kFalseType); return EndValue(write_bool(b)); }
    bool emplace_int32(int i)             { Prefix(kNumberType); return EndValue(write_int(i)); }
    bool emplace_uint32(unsigned u)       { Prefix(kNumberType); return EndValue(write_uint(u)); }
    bool emplace_int64(int64_t i64)     { Prefix(kNumberType); return EndValue(write_int64(i64)); }
    bool emplace_uint64(uint64_t u64)   { Prefix(kNumberType); return EndValue(write_uint64(u64)); }

    //! Writes the given \c double value to the stream
    /*!
        \param d The value to be written.
        \return Whether it is succeed.
    */
    bool emplace_double(double d)       { Prefix(kNumberType); return EndValue(write_double(d)); }

    bool raw_number(const char_type* str, SizeType length, bool copy = false) {
        MERAK_JSON_ASSERT(str != 0);
        (void)copy;
        Prefix(kNumberType);
        return EndValue(write_string(str, length));
    }

    bool emplace_string(const char_type* str, SizeType length, bool copy = false) {
        MERAK_JSON_ASSERT(str != 0);
        (void)copy;
        Prefix(kStringType);
        return EndValue(write_string(str, length));
    }


    bool emplace_string(const std::basic_string<char_type>& str) {
        return emplace_string(str.data(), SizeType(str.size()));
    }


    bool start_object() {
        Prefix(kObjectType);
        new (level_stack_.template Push<Level>()) Level(false);
        return WriteStartObject();
    }

    bool emplace_key(const char_type* str, SizeType length, bool copy = false) { return emplace_string(str, length, copy); }


    bool emplace_key(const std::basic_string<char_type>& str)
    {
      return emplace_key(str.data(), SizeType(str.size()));
    }


    bool end_object(SizeType memberCount = 0) {
        (void)memberCount;
        MERAK_JSON_ASSERT(level_stack_.GetSize() >= sizeof(Level)); // not inside an Object
        MERAK_JSON_ASSERT(!level_stack_.template Top<Level>()->inArray); // currently inside an Array, not Object
        MERAK_JSON_ASSERT(0 == level_stack_.template Top<Level>()->valueCount % 2); // Object has a Key without a Value
        level_stack_.template Pop<Level>(1);
        return EndValue(WriteEndObject());
    }

    bool start_array() {
        Prefix(kArrayType);
        new (level_stack_.template Push<Level>()) Level(true);
        return WriteStartArray();
    }

    bool end_array(SizeType elementCount = 0) {
        (void)elementCount;
        MERAK_JSON_ASSERT(level_stack_.GetSize() >= sizeof(Level));
        MERAK_JSON_ASSERT(level_stack_.template Top<Level>()->inArray);
        level_stack_.template Pop<Level>(1);
        return EndValue(WriteEndArray());
    }
    //@}

    /*! @name Convenience extensions */
    //@{

    //! Simpler but slower overload.
    bool emplace_string(const char_type* const& str) { return emplace_string(str, internal::StrLen(str)); }
    bool emplace_key(const char_type* const& str) { return emplace_key(str, internal::StrLen(str)); }
    
    //@}

    //! Write a raw JSON value.
    /*!
        For user to write a stringified JSON as a value.

        \param json A well-formed JSON value. It should not contain null character within [0, length - 1] range.
        \param length Length of the json.
        \param type Type of the root of json.
    */
    bool raw_value(const char_type* json, size_t length, Type type) {
        MERAK_JSON_ASSERT(json != 0);
        Prefix(type);
        return EndValue(WriteRawValue(json, length));
    }

    //! Flush the output stream.
    /*!
        Allows the user to flush the output stream immediately.
     */
    Writer &flush() {
        os_->flush();
        return *this;
    }

    static const size_t kDefaultLevelDepth = 32;

protected:
    //! Information for each nested level
    struct Level {
        Level(bool inArray_) : valueCount(0), inArray(inArray_) {}
        size_t valueCount;  //!< number of values in this level
        bool inArray;       //!< true if in array, otherwise in object
    };

    bool WriteNull()  {
        PutReserve(*os_, 4);
        PutUnsafe(*os_, 'n'); PutUnsafe(*os_, 'u'); PutUnsafe(*os_, 'l'); PutUnsafe(*os_, 'l'); return true;
    }

    bool write_bool(bool b)  {
        if (b) {
            PutReserve(*os_, 4);
            PutUnsafe(*os_, 't'); PutUnsafe(*os_, 'r'); PutUnsafe(*os_, 'u'); PutUnsafe(*os_, 'e');
        }
        else {
            PutReserve(*os_, 5);
            PutUnsafe(*os_, 'f'); PutUnsafe(*os_, 'a'); PutUnsafe(*os_, 'l'); PutUnsafe(*os_, 's'); PutUnsafe(*os_, 'e');
        }
        return true;
    }

    bool write_int(int i) {
        char buffer[11];
        const char* end = internal::i32toa(i, buffer);
        PutReserve(*os_, static_cast<size_t>(end - buffer));
        for (const char* p = buffer; p != end; ++p)
            PutUnsafe(*os_, static_cast<typename OutputStream::char_type>(*p));
        return true;
    }

    bool write_uint(unsigned u) {
        char buffer[10];
        const char* end = internal::u32toa(u, buffer);
        PutReserve(*os_, static_cast<size_t>(end - buffer));
        for (const char* p = buffer; p != end; ++p)
            PutUnsafe(*os_, static_cast<typename OutputStream::char_type>(*p));
        return true;
    }

    bool write_int64(int64_t i64) {
        char buffer[21];
        const char* end = internal::i64toa(i64, buffer);
        PutReserve(*os_, static_cast<size_t>(end - buffer));
        for (const char* p = buffer; p != end; ++p)
            PutUnsafe(*os_, static_cast<typename OutputStream::char_type>(*p));
        return true;
    }

    bool write_uint64(uint64_t u64) {
        char buffer[20];
        char* end = internal::u64toa(u64, buffer);
        PutReserve(*os_, static_cast<size_t>(end - buffer));
        for (char* p = buffer; p != end; ++p)
            PutUnsafe(*os_, static_cast<typename OutputStream::char_type>(*p));
        return true;
    }

    bool write_double(double d) {
        if (internal::Double(d).IsNanOrInf()) {
            if (!(writeFlags & kWriteNanAndInfFlag) && !(writeFlags & kWriteNanAndInfNullFlag))
                return false;
            if (writeFlags & kWriteNanAndInfNullFlag) {
                PutReserve(*os_, 4);
                PutUnsafe(*os_, 'n'); PutUnsafe(*os_, 'u'); PutUnsafe(*os_, 'l'); PutUnsafe(*os_, 'l');
                return true;
            }
            if (internal::Double(d).IsNan()) {
                PutReserve(*os_, 3);
                PutUnsafe(*os_, 'N'); PutUnsafe(*os_, 'a'); PutUnsafe(*os_, 'N');
                return true;
            }
            if (internal::Double(d).Sign()) {
                PutReserve(*os_, 9);
                PutUnsafe(*os_, '-');
            }
            else
                PutReserve(*os_, 8);
            PutUnsafe(*os_, 'I'); PutUnsafe(*os_, 'n'); PutUnsafe(*os_, 'f');
            PutUnsafe(*os_, 'i'); PutUnsafe(*os_, 'n'); PutUnsafe(*os_, 'i'); PutUnsafe(*os_, 't'); PutUnsafe(*os_, 'y');
            return true;
        }

        char buffer[25];
        char* end = internal::dtoa(d, buffer, maxDecimalPlaces_);
        PutReserve(*os_, static_cast<size_t>(end - buffer));
        for (char* p = buffer; p != end; ++p)
            PutUnsafe(*os_, static_cast<typename OutputStream::char_type>(*p));
        return true;
    }

    bool write_string(const char_type *str, SizeType length) {
        //if TargetEncoding support Unicode
        //and SourceEncoding and TargetEncoding are the same type
        //just use memcpy to improve efficiency
        // HasPuts<OutputStream>::value &&
        if constexpr (!(kWriteValidateEncodingFlag & writeFlags)&&  TargetEncoding::supportUnicode && std::is_same_v<SourceEncoding, TargetEncoding>) {
            static const char hexDigits[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                                               '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
            static const char escape[256] = {
#define ESCAPE_ZERO_16 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
                    //0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F
                    'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'b', 't', 'n', 'u', 'f', 'r', 'u', 'u', // 00
                    'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', // 10
                    0, 0, '"', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 20
                    ESCAPE_ZERO_16, ESCAPE_ZERO_16,                                                 // 30~4F
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '\\', 0, 0, 0, // 50
                    ESCAPE_ZERO_16, ESCAPE_ZERO_16, ESCAPE_ZERO_16, ESCAPE_ZERO_16, ESCAPE_ZERO_16,
                    ESCAPE_ZERO_16, ESCAPE_ZERO_16, ESCAPE_ZERO_16, ESCAPE_ZERO_16, ESCAPE_ZERO_16  // 60~FF
#undef ESCAPE_ZERO_16
            };
            os_->put('\"');
            size_t index = 0;
            size_t pos = 0;
            while (pos < length) {
                char_type c = str[pos];
                if ((sizeof(char_type) == 1 || (unsigned) c < 256) && escape[(unsigned char) c]) {
                    os_->write(str + index, pos - index);
                    index = pos + 1;
                    os_->put('\\');
                    os_->put(escape[(unsigned char) str[pos]]);
                    if (escape[(unsigned char) str[pos]] == 'u') {
                        os_->put('0');
                        os_->put('0');
                        os_->put(hexDigits[(unsigned char) str[pos] >> 4]);
                        os_->put(hexDigits[(unsigned char) str[pos] & 0xF]);
                    }
                }
                pos++;
            }
            if (index < length) {
                os_->write(str + index, length - index);
            }
            os_->put('\"');
            return true;
        } else {
            return write_string_slow(str, length);
        }
    }


    bool write_string_slow(const char_type* str, SizeType length)  {
        static const typename OutputStream::char_type hexDigits[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
        static const char escape[256] = {
#define Z16 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
            //0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F
            'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'b', 't', 'n', 'u', 'f', 'r', 'u', 'u', // 00
            'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', // 10
              0,   0, '"',   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // 20
            Z16, Z16,                                                                       // 30~4F
              0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,'\\',   0,   0,   0, // 50
            Z16, Z16, Z16, Z16, Z16, Z16, Z16, Z16, Z16, Z16                                // 60~FF
#undef Z16
        };

        if (TargetEncoding::supportUnicode)
            PutReserve(*os_, 2 + length * 6); // "\uxxxx..."
        else
            PutReserve(*os_, 2 + length * 12);  // "\uxxxx\uyyyy..."

        PutUnsafe(*os_, '\"');
        GenericStringStream<SourceEncoding> is(str);
        while (ScanWriteUnescapedString(is, length)) {
            const char_type c = is.peek();
            if (!TargetEncoding::supportUnicode && static_cast<unsigned>(c) >= 0x80) {
                // Unicode escaping
                unsigned codepoint;
                if (MERAK_JSON_UNLIKELY(!SourceEncoding::Decode(is, &codepoint)))
                    return false;
                PutUnsafe(*os_, '\\');
                PutUnsafe(*os_, 'u');
                if (codepoint <= 0xD7FF || (codepoint >= 0xE000 && codepoint <= 0xFFFF)) {
                    PutUnsafe(*os_, hexDigits[(codepoint >> 12) & 15]);
                    PutUnsafe(*os_, hexDigits[(codepoint >>  8) & 15]);
                    PutUnsafe(*os_, hexDigits[(codepoint >>  4) & 15]);
                    PutUnsafe(*os_, hexDigits[(codepoint      ) & 15]);
                }
                else {
                    MERAK_JSON_ASSERT(codepoint >= 0x010000 && codepoint <= 0x10FFFF);
                    // Surrogate pair
                    unsigned s = codepoint - 0x010000;
                    unsigned lead = (s >> 10) + 0xD800;
                    unsigned trail = (s & 0x3FF) + 0xDC00;
                    PutUnsafe(*os_, hexDigits[(lead >> 12) & 15]);
                    PutUnsafe(*os_, hexDigits[(lead >>  8) & 15]);
                    PutUnsafe(*os_, hexDigits[(lead >>  4) & 15]);
                    PutUnsafe(*os_, hexDigits[(lead      ) & 15]);
                    PutUnsafe(*os_, '\\');
                    PutUnsafe(*os_, 'u');
                    PutUnsafe(*os_, hexDigits[(trail >> 12) & 15]);
                    PutUnsafe(*os_, hexDigits[(trail >>  8) & 15]);
                    PutUnsafe(*os_, hexDigits[(trail >>  4) & 15]);
                    PutUnsafe(*os_, hexDigits[(trail      ) & 15]);                    
                }
            }
            else if ((sizeof(char_type) == 1 || static_cast<unsigned>(c) < 256) && MERAK_JSON_UNLIKELY(escape[static_cast<unsigned char>(c)]))  {
                is.get();
                PutUnsafe(*os_, '\\');
                PutUnsafe(*os_, static_cast<typename OutputStream::char_type>(escape[static_cast<unsigned char>(c)]));
                if (escape[static_cast<unsigned char>(c)] == 'u') {
                    PutUnsafe(*os_, '0');
                    PutUnsafe(*os_, '0');
                    PutUnsafe(*os_, hexDigits[static_cast<unsigned char>(c) >> 4]);
                    PutUnsafe(*os_, hexDigits[static_cast<unsigned char>(c) & 0xF]);
                }
            }
            else if (MERAK_JSON_UNLIKELY(!(writeFlags & kWriteValidateEncodingFlag ?
                Transcoder<SourceEncoding, TargetEncoding>::Validate(is, *os_) :
                Transcoder<SourceEncoding, TargetEncoding>::TranscodeUnsafe(is, *os_))))
                return false;
        }
        PutUnsafe(*os_, '\"');
        return true;
    }

    bool ScanWriteUnescapedString(GenericStringStream<SourceEncoding>& is, size_t length) {
        return MERAK_JSON_LIKELY(is.tellg() < length);
    }

    bool WriteStartObject() { os_->put('{'); return true; }
    bool WriteEndObject()   { os_->put('}'); return true; }
    bool WriteStartArray()  { os_->put('['); return true; }
    bool WriteEndArray()    { os_->put(']'); return true; }

    bool WriteRawValue(const char_type* json, size_t length) {
        PutReserve(*os_, length);
        GenericStringStream<SourceEncoding> is(json);
        while (MERAK_JSON_LIKELY(is.tellg() < length)) {
            MERAK_JSON_ASSERT(is.peek() != '\0');
            if (MERAK_JSON_UNLIKELY(!(writeFlags & kWriteValidateEncodingFlag ?
                Transcoder<SourceEncoding, TargetEncoding>::Validate(is, *os_) :
                Transcoder<SourceEncoding, TargetEncoding>::TranscodeUnsafe(is, *os_))))
                return false;
        }
        return true;
    }

    void Prefix(Type type) {
        (void)type;
        if (MERAK_JSON_LIKELY(level_stack_.GetSize() != 0)) { // this value is not at root
            Level* level = level_stack_.template Top<Level>();
            if (level->valueCount > 0) {
                if (level->inArray) 
                    os_->put(','); // add comma if it is not the first element in array
                else  // in object
                    os_->put((level->valueCount % 2 == 0) ? ',' : ':');
            }
            if (!level->inArray && level->valueCount % 2 == 0)
                MERAK_JSON_ASSERT(type == kStringType);  // if it's in object, then even number should be a name
            level->valueCount++;
        }
        else {
            MERAK_JSON_ASSERT(!hasRoot_);    // Should only has one and only one root.
            hasRoot_ = true;
        }
    }

    // Flush the value if it is the top level one.
    bool EndValue(bool ret) {
        if (MERAK_JSON_UNLIKELY(level_stack_.empty()))   // end of json text
            flush();
        return ret;
    }

    OutputStream* os_;
    internal::Stack<StackAllocator> level_stack_;
    int maxDecimalPlaces_;
    bool hasRoot_;

private:
    // Prohibit copy constructor & assignment operator.
    Writer(const Writer&);
    Writer& operator=(const Writer&);
};

// Full specialization for StringStream to prevent memory copying

template<>
inline bool Writer<StringBuffer>::write_int(int i) {
    char *buffer = os_->Push(11);
    const char* end = internal::i32toa(i, buffer);
    os_->Pop(static_cast<size_t>(11 - (end - buffer)));
    return true;
}

template<>
inline bool Writer<StringBuffer>::write_uint(unsigned u) {
    char *buffer = os_->Push(10);
    const char* end = internal::u32toa(u, buffer);
    os_->Pop(static_cast<size_t>(10 - (end - buffer)));
    return true;
}

template<>
inline bool Writer<StringBuffer>::write_int64(int64_t i64) {
    char *buffer = os_->Push(21);
    const char* end = internal::i64toa(i64, buffer);
    os_->Pop(static_cast<size_t>(21 - (end - buffer)));
    return true;
}

template<>
inline bool Writer<StringBuffer>::write_uint64(uint64_t u) {
    char *buffer = os_->Push(20);
    const char* end = internal::u64toa(u, buffer);
    os_->Pop(static_cast<size_t>(20 - (end - buffer)));
    return true;
}

template<>
inline bool Writer<StringBuffer>::write_double(double d) {
    if (internal::Double(d).IsNanOrInf()) {
        // Note: This code path can only be reached if (MERAK_JSON_WRITE_DEFAULT_FLAGS & kWriteNanAndInfFlag).
        if (!(kWriteDefaultFlags & kWriteNanAndInfFlag))
            return false;
        if (kWriteDefaultFlags & kWriteNanAndInfNullFlag) {
            PutReserve(*os_, 4);
            PutUnsafe(*os_, 'n'); PutUnsafe(*os_, 'u'); PutUnsafe(*os_, 'l'); PutUnsafe(*os_, 'l');
            return true;
        }
        if (internal::Double(d).IsNan()) {
            PutReserve(*os_, 3);
            PutUnsafe(*os_, 'N'); PutUnsafe(*os_, 'a'); PutUnsafe(*os_, 'N');
            return true;
        }
        if (internal::Double(d).Sign()) {
            PutReserve(*os_, 9);
            PutUnsafe(*os_, '-');
        }
        else
            PutReserve(*os_, 8);
        PutUnsafe(*os_, 'I'); PutUnsafe(*os_, 'n'); PutUnsafe(*os_, 'f');
        PutUnsafe(*os_, 'i'); PutUnsafe(*os_, 'n'); PutUnsafe(*os_, 'i'); PutUnsafe(*os_, 't'); PutUnsafe(*os_, 'y');
        return true;
    }
    
    char *buffer = os_->Push(25);
    char* end = internal::dtoa(d, buffer, maxDecimalPlaces_);
    os_->Pop(static_cast<size_t>(25 - (end - buffer)));
    return true;
}

#if defined(MERAK_JSON_SSE2) || defined(MERAK_JSON_SSE42)
template<>
inline bool Writer<StringBuffer>::ScanWriteUnescapedString(StringStream& is, size_t length) {
    if (length < 16)
        return MERAK_JSON_LIKELY(is.tellg() < length);

    if (!MERAK_JSON_LIKELY(is.tellg() < length))
        return false;

    const char* p = is.src_;
    const char* end = is.head_ + length;
    const char* nextAligned = reinterpret_cast<const char*>((reinterpret_cast<size_t>(p) + 15) & static_cast<size_t>(~15));
    const char* endAligned = reinterpret_cast<const char*>(reinterpret_cast<size_t>(end) & static_cast<size_t>(~15));
    if (nextAligned > end)
        return true;

    while (p != nextAligned)
        if (*p < 0x20 || *p == '\"' || *p == '\\') {
            is.src_ = p;
            return MERAK_JSON_LIKELY(is.tellg() < length);
        }
        else
            os_->PutUnsafe(*p++);

    // The rest of string using SIMD
    static const char dquote[16] = { '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"' };
    static const char bslash[16] = { '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\' };
    static const char space[16]  = { 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F };
    const __m128i dq = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&dquote[0]));
    const __m128i bs = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&bslash[0]));
    const __m128i sp = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&space[0]));

    for (; p != endAligned; p += 16) {
        const __m128i s = _mm_load_si128(reinterpret_cast<const __m128i *>(p));
        const __m128i t1 = _mm_cmpeq_epi8(s, dq);
        const __m128i t2 = _mm_cmpeq_epi8(s, bs);
        const __m128i t3 = _mm_cmpeq_epi8(_mm_max_epu8(s, sp), sp); // s < 0x20 <=> max(s, 0x1F) == 0x1F
        const __m128i x = _mm_or_si128(_mm_or_si128(t1, t2), t3);
        unsigned short r = static_cast<unsigned short>(_mm_movemask_epi8(x));
        if (MERAK_JSON_UNLIKELY(r != 0)) {   // some of characters is escaped
            SizeType len;
#ifdef _MSC_VER         // Find the index of first escaped
            unsigned long offset;
            _BitScanForward(&offset, r);
            len = offset;
#else
            len = static_cast<SizeType>(__builtin_ffs(r) - 1);
#endif
            char* q = reinterpret_cast<char*>(os_->PushUnsafe(len));
            for (size_t i = 0; i < len; i++)
                q[i] = p[i];

            p += len;
            break;
        }
        _mm_storeu_si128(reinterpret_cast<__m128i *>(os_->PushUnsafe(16)), s);
    }

    is.src_ = p;
    return MERAK_JSON_LIKELY(is.tellg() < length);
}
#elif defined(MERAK_JSON_NEON)
template<>
inline bool Writer<StringBuffer>::ScanWriteUnescapedString(StringStream& is, size_t length) {
    if (length < 16)
        return MERAK_JSON_LIKELY(is.tellg() < length);

    if (!MERAK_JSON_LIKELY(is.tellg() < length))
        return false;

    const char* p = is.src_;
    const char* end = is.head_ + length;
    const char* nextAligned = reinterpret_cast<const char*>((reinterpret_cast<size_t>(p) + 15) & static_cast<size_t>(~15));
    const char* endAligned = reinterpret_cast<const char*>(reinterpret_cast<size_t>(end) & static_cast<size_t>(~15));
    if (nextAligned > end)
        return true;

    while (p != nextAligned)
        if (*p < 0x20 || *p == '\"' || *p == '\\') {
            is.src_ = p;
            return MERAK_JSON_LIKELY(is.tellg() < length);
        }
        else
            os_->PutUnsafe(*p++);

    // The rest of string using SIMD
    const uint8x16_t s0 = vmovq_n_u8('"');
    const uint8x16_t s1 = vmovq_n_u8('\\');
    const uint8x16_t s2 = vmovq_n_u8('\b');
    const uint8x16_t s3 = vmovq_n_u8(32);

    for (; p != endAligned; p += 16) {
        const uint8x16_t s = vld1q_u8(reinterpret_cast<const uint8_t *>(p));
        uint8x16_t x = vceqq_u8(s, s0);
        x = vorrq_u8(x, vceqq_u8(s, s1));
        x = vorrq_u8(x, vceqq_u8(s, s2));
        x = vorrq_u8(x, vcltq_u8(s, s3));

        x = vrev64q_u8(x);                     // Rev in 64
        uint64_t low = vgetq_lane_u64(vreinterpretq_u64_u8(x), 0);   // extract
        uint64_t high = vgetq_lane_u64(vreinterpretq_u64_u8(x), 1);  // extract

        SizeType len = 0;
        bool escaped = false;
        if (low == 0) {
            if (high != 0) {
                uint32_t lz = internal::clzll(high);
                len = 8 + (lz >> 3);
                escaped = true;
            }
        } else {
            uint32_t lz = internal::clzll(low);
            len = lz >> 3;
            escaped = true;
        }
        if (MERAK_JSON_UNLIKELY(escaped)) {   // some of characters is escaped
            char* q = reinterpret_cast<char*>(os_->PushUnsafe(len));
            for (size_t i = 0; i < len; i++)
                q[i] = p[i];

            p += len;
            break;
        }
        vst1q_u8(reinterpret_cast<uint8_t *>(os_->PushUnsafe(16)), s);
    }

    is.src_ = p;
    return MERAK_JSON_LIKELY(is.tellg() < length);
}
#endif // MERAK_JSON_NEON

}  // namespace merak::json

#if defined(_MSC_VER) || defined(__clang__)
MERAK_JSON_DIAG_POP
#endif

#endif // MERAK_JSON_WRITER_H_
