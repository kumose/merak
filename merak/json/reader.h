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

#ifndef MERAK_JSON_READER_H_
#define MERAK_JSON_READER_H_

/*! \file reader.h */

#include <merak/json/allocators.h>
#include <merak/json/stream.h>
#include <merak/json/encodedstream.h>
#include <merak/json/internal/clzll.h>
#include <merak/json/internal/meta.h>
#include <merak/json/internal/stack.h>
#include <merak/json/internal/strtod.h>
#include <limits>

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
MERAK_JSON_DIAG_OFF(old-style-cast)
MERAK_JSON_DIAG_OFF(padded)
MERAK_JSON_DIAG_OFF(switch-enum)
#elif defined(_MSC_VER)
MERAK_JSON_DIAG_PUSH
MERAK_JSON_DIAG_OFF(4127)  // conditional expression is constant
MERAK_JSON_DIAG_OFF(4702)  // unreachable code
#endif

#ifdef __GNUC__
MERAK_JSON_DIAG_PUSH
MERAK_JSON_DIAG_OFF(effc++)
#endif

//!@cond MERAK_JSON_HIDDEN_FROM_DOXYGEN
#define MERAK_JSON_NOTHING /* deliberately empty */
#ifndef MERAK_JSON_PARSE_ERROR_EARLY_RETURN
#define MERAK_JSON_PARSE_ERROR_EARLY_RETURN(value) \
    MERAK_JSON_MULTILINEMACRO_BEGIN \
    if (MERAK_JSON_UNLIKELY(has_parse_error())) { return value; } \
    MERAK_JSON_MULTILINEMACRO_END
#endif
#define MERAK_JSON_PARSE_ERROR_EARLY_RETURN_VOID \
    MERAK_JSON_PARSE_ERROR_EARLY_RETURN(MERAK_JSON_NOTHING)
//!@endcond

/*! \def MERAK_JSON_PARSE_ERROR_NORETURN
    \ingroup MERAK_JSON_ERRORS
    \brief Macro to indicate a parse error.
    \param parseErrorCode \ref merak::json::ParseErrorCode of the error
    \param offset  position of the error in JSON input (\c size_t)

    This macros can be used as a customization point for the internal
    error handling mechanism of RapidJSON.

    A common usage model is to throw an exception instead of requiring the
    caller to explicitly check the \ref merak::json::GenericReader::parse's
    return value:

    \code
    #define MERAK_JSON_PARSE_ERROR_NORETURN(parseErrorCode,offset) \
       throw ParseException(parseErrorCode, #parseErrorCode, offset)

    #include <stdexcept>               // std::runtime_error
    #include <merak/json/error/error.h> // merak::json::ParseResult

    struct ParseException : std::runtime_error, merak::json::ParseResult {
      ParseException(merak::json::ParseErrorCode code, const char* msg, size_t offset)
        : std::runtime_error(msg), ParseResult(code, offset) {}
    };

    #include <merak/json/reader.h>
    \endcode

    \see MERAK_JSON_PARSE_ERROR, merak::json::GenericReader::parse
 */
#ifndef MERAK_JSON_PARSE_ERROR_NORETURN
#define MERAK_JSON_PARSE_ERROR_NORETURN(parseErrorCode, offset) \
    MERAK_JSON_MULTILINEMACRO_BEGIN \
    MERAK_JSON_ASSERT(!has_parse_error()); /* Error can only be assigned once */ \
    SetParseError(parseErrorCode, offset); \
    MERAK_JSON_MULTILINEMACRO_END
#endif

/*! \def MERAK_JSON_PARSE_ERROR
    \ingroup MERAK_JSON_ERRORS
    \brief (Internal) macro to indicate and handle a parse error.
    \param parseErrorCode \ref merak::json::ParseErrorCode of the error
    \param offset  position of the error in JSON input (\c size_t)

    Invokes MERAK_JSON_PARSE_ERROR_NORETURN and stops the parsing.

    \see MERAK_JSON_PARSE_ERROR_NORETURN
    \hideinitializer
 */
#ifndef MERAK_JSON_PARSE_ERROR
#define MERAK_JSON_PARSE_ERROR(parseErrorCode, offset) \
    MERAK_JSON_MULTILINEMACRO_BEGIN \
    MERAK_JSON_PARSE_ERROR_NORETURN(parseErrorCode, offset); \
    MERAK_JSON_PARSE_ERROR_EARLY_RETURN_VOID; \
    MERAK_JSON_MULTILINEMACRO_END
#endif

#include <merak/json/error/error.h>

namespace merak::json {

///////////////////////////////////////////////////////////////////////////////
// ParseFlag

/*! \def MERAK_JSON_PARSE_DEFAULT_FLAGS
    \ingroup MERAK_JSON_CONFIG
    \brief User-defined kParseDefaultFlags definition.

    User can define this as any \c ParseFlag combinations.
*/
#ifndef MERAK_JSON_PARSE_DEFAULT_FLAGS
#define MERAK_JSON_PARSE_DEFAULT_FLAGS kParseNoFlags
#endif

    //! Combination of parseFlags
    /*! \see Reader::parse, Document::parse, Document::parse_stream
     */
    enum ParseFlag {
        /// No flags are set.
        kParseNoFlags = 0,
        /// Validate encoding of JSON strings.
        kParseValidateEncodingFlag = 1,
        /// Iterative(constant complexity in terms of function call stack size) parsing.
        kParseIterativeFlag = 2,
        /// After parsing a complete JSON root from stream, stop further processing the rest of stream. When this flag is used, parser will not generate kParseErrorDocumentRootNotSingular error.
        kParseStopWhenDoneFlag = 4,
        /// parse number in full precision (but slower).
        kParseFullPrecisionFlag = 8,
        /// Allow one-line (//) and multi-line (/**/) comments.
        kParseCommentsFlag = 16,
        /// parse all numbers (ints/doubles) as strings.
        kParseNumbersAsStringsFlag = 32,
        /// Allow trailing commas at the end of objects and arrays.
        kParseTrailingCommasFlag = 64,
        /// Allow parsing NaN, Inf, Infinity, -Inf and -Infinity as doubles.
        kParseNanAndInfFlag = 128,
        /// Allow escaped apostrophe in strings.
        kParseEscapedApostropheFlag = 256,
        /// Default parse flags. Can be customized by defining MERAK_JSON_PARSE_DEFAULT_FLAGS
        kParseDefaultFlags = MERAK_JSON_PARSE_DEFAULT_FLAGS
    };

    ///////////////////////////////////////////////////////////////////////////////
    // Handler

    /*! \class merak::json::Handler
        \brief Concept for receiving events from GenericReader upon parsing.
        The functions return true if no error occurs. If they return false,
        the event publisher should terminate the process.
    \code
    concept Handler {
        typename char_type;

        bool emplace_null();
        bool emplace_bool(bool b);
        bool emplace_int32(int i);
        bool emplace_uint32(unsigned i);
        bool emplace_int64(int64_t i);
        bool emplace_uint64(uint64_t i);
        bool emplace_double(double d);
        /// enabled via kParseNumbersAsStringsFlag, string is not null-terminated (use length)
        bool raw_number(const char_type* str, SizeType length, bool copy);
        bool emplace_string(const char_type* str, SizeType length, bool copy);
        bool start_object();
        bool emplace_key(const char_type* str, SizeType length, bool copy);
        bool end_object(SizeType memberCount);
        bool start_array();
        bool end_array(SizeType elementCount);
    };
    \endcode
    */
    ///////////////////////////////////////////////////////////////////////////////
    // BaseReaderHandler

    //! Default implementation of Handler.
    /*! This can be used as base class of any reader handler.
        \note implements Handler concept
    */
    template<typename Encoding = UTF8<>, typename Derived = void>
    struct BaseReaderHandler {
        typedef typename Encoding::char_type char_type;

        typedef typename internal::SelectIf<internal::IsSame<Derived, void>, BaseReaderHandler, Derived>::Type Override;

        bool Default() { return true; }

        bool emplace_null() { return static_cast<Override &>(*this).Default(); }

        bool emplace_bool(bool) { return static_cast<Override &>(*this).Default(); }

        bool emplace_int32(int) { return static_cast<Override &>(*this).Default(); }

        bool emplace_uint32(unsigned) { return static_cast<Override &>(*this).Default(); }

        bool emplace_int64(int64_t) { return static_cast<Override &>(*this).Default(); }

        bool emplace_uint64(uint64_t) { return static_cast<Override &>(*this).Default(); }

        bool emplace_double(double) { return static_cast<Override &>(*this).Default(); }

        /// enabled via kParseNumbersAsStringsFlag, string is not null-terminated (use length)
        bool raw_number(const char_type *str, SizeType len, bool copy) {
            return static_cast<Override &>(*this).emplace_string(str, len, copy);
        }

        bool emplace_string(const char_type *, SizeType, bool) { return static_cast<Override &>(*this).Default(); }

        bool start_object() { return static_cast<Override &>(*this).Default(); }

        bool emplace_key(const char_type *str, SizeType len, bool copy) {
            return static_cast<Override &>(*this).emplace_string(str, len, copy);
        }

        bool end_object(SizeType) { return static_cast<Override &>(*this).Default(); }

        bool start_array() { return static_cast<Override &>(*this).Default(); }

        bool end_array(SizeType) { return static_cast<Override &>(*this).Default(); }
    };

    ///////////////////////////////////////////////////////////////////////////////
    // StreamLocalCopy

    namespace internal {

        template<typename Stream, int = StreamTraits<Stream>::copyOptimization>
        class StreamLocalCopy;

//! Do copy optimization.
        template<typename Stream>
        class StreamLocalCopy<Stream, 1> {
        public:
            StreamLocalCopy(Stream &original) : s(original), original_(original) {}

            ~StreamLocalCopy() { original_ = s; }

            Stream s;

        private:
            StreamLocalCopy &operator=(const StreamLocalCopy &) /* = delete */;

            Stream &original_;
        };

//! Keep reference.
        template<typename Stream>
        class StreamLocalCopy<Stream, 0> {
        public:
            StreamLocalCopy(Stream &original) : s(original) {}

            Stream &s;

        private:
            StreamLocalCopy &operator=(const StreamLocalCopy &) /* = delete */;
        };

    } // namespace internal

///////////////////////////////////////////////////////////////////////////////
// SkipWhitespace

//! Skip the JSON white spaces in a stream.
/*! \param is A input stream for skipping white spaces.
    \note This function has SSE2/SSE4.2 specialization.
*/
    template<typename InputStream>
    void SkipWhitespace(InputStream &is) {
        internal::StreamLocalCopy<InputStream> copy(is);
        InputStream &s(copy.s);

        typename InputStream::char_type c;
        while ((c = s.peek()) == ' ' || c == '\n' || c == '\r' || c == '\t')
            s.get();
    }

    inline const char *SkipWhitespace(const char *p, const char *end) {
        while (p != end && (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t'))
            ++p;
        return p;
    }

#ifdef MERAK_JSON_SSE42
    //! Skip whitespace with SSE 4.2 pcmpistrm instruction, testing 16 8-byte characters at once.
    inline const char *SkipWhitespace_SIMD(const char* p) {
        // Fast return for single non-whitespace
        if (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t')
            ++p;
        else
            return p;

        // 16-byte align to the next boundary
        const char* nextAligned = reinterpret_cast<const char*>((reinterpret_cast<size_t>(p) + 15) & static_cast<size_t>(~15));
        while (p != nextAligned)
            if (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t')
                ++p;
            else
                return p;

        // The rest of string using SIMD
        static const char whitespace[16] = " \n\r\t";
        const __m128i w = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&whitespace[0]));

        for (;; p += 16) {
            const __m128i s = _mm_load_si128(reinterpret_cast<const __m128i *>(p));
            const int r = _mm_cmpistri(w, s, _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_ANY | _SIDD_LEAST_SIGNIFICANT | _SIDD_NEGATIVE_POLARITY);
            if (r != 16)    // some of characters is non-whitespace
                return p + r;
        }
    }

    inline const char *SkipWhitespace_SIMD(const char* p, const char* end) {
        // Fast return for single non-whitespace
        if (p != end && (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t'))
            ++p;
        else
            return p;

        // The middle of string using SIMD
        static const char whitespace[16] = " \n\r\t";
        const __m128i w = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&whitespace[0]));

        for (; p <= end - 16; p += 16) {
            const __m128i s = _mm_loadu_si128(reinterpret_cast<const __m128i *>(p));
            const int r = _mm_cmpistri(w, s, _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_ANY | _SIDD_LEAST_SIGNIFICANT | _SIDD_NEGATIVE_POLARITY);
            if (r != 16)    // some of characters is non-whitespace
                return p + r;
        }

        return SkipWhitespace(p, end);
    }

#elif defined(MERAK_JSON_SSE2)

    //! Skip whitespace with SSE2 instructions, testing 16 8-byte characters at once.
    inline const char *SkipWhitespace_SIMD(const char* p) {
        // Fast return for single non-whitespace
        if (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t')
            ++p;
        else
            return p;

        // 16-byte align to the next boundary
        const char* nextAligned = reinterpret_cast<const char*>((reinterpret_cast<size_t>(p) + 15) & static_cast<size_t>(~15));
        while (p != nextAligned)
            if (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t')
                ++p;
            else
                return p;

        // The rest of string
#define C16(c) { c, c, c, c, c, c, c, c, c, c, c, c, c, c, c, c }
        static const char whitespaces[4][16] = { C16(' '), C16('\n'), C16('\r'), C16('\t') };
#undef C16

        const __m128i w0 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&whitespaces[0][0]));
        const __m128i w1 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&whitespaces[1][0]));
        const __m128i w2 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&whitespaces[2][0]));
        const __m128i w3 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&whitespaces[3][0]));

        for (;; p += 16) {
            const __m128i s = _mm_load_si128(reinterpret_cast<const __m128i *>(p));
            __m128i x = _mm_cmpeq_epi8(s, w0);
            x = _mm_or_si128(x, _mm_cmpeq_epi8(s, w1));
            x = _mm_or_si128(x, _mm_cmpeq_epi8(s, w2));
            x = _mm_or_si128(x, _mm_cmpeq_epi8(s, w3));
            unsigned short r = static_cast<unsigned short>(~_mm_movemask_epi8(x));
            if (r != 0) {   // some of characters may be non-whitespace
#ifdef _MSC_VER         // Find the index of first non-whitespace
                unsigned long offset;
                _BitScanForward(&offset, r);
                return p + offset;
#else
                return p + __builtin_ffs(r) - 1;
#endif
            }
        }
    }

    inline const char *SkipWhitespace_SIMD(const char* p, const char* end) {
        // Fast return for single non-whitespace
        if (p != end && (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t'))
            ++p;
        else
            return p;

        // The rest of string
#define C16(c) { c, c, c, c, c, c, c, c, c, c, c, c, c, c, c, c }
        static const char whitespaces[4][16] = { C16(' '), C16('\n'), C16('\r'), C16('\t') };
#undef C16

        const __m128i w0 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&whitespaces[0][0]));
        const __m128i w1 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&whitespaces[1][0]));
        const __m128i w2 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&whitespaces[2][0]));
        const __m128i w3 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&whitespaces[3][0]));

        for (; p <= end - 16; p += 16) {
            const __m128i s = _mm_loadu_si128(reinterpret_cast<const __m128i *>(p));
            __m128i x = _mm_cmpeq_epi8(s, w0);
            x = _mm_or_si128(x, _mm_cmpeq_epi8(s, w1));
            x = _mm_or_si128(x, _mm_cmpeq_epi8(s, w2));
            x = _mm_or_si128(x, _mm_cmpeq_epi8(s, w3));
            unsigned short r = static_cast<unsigned short>(~_mm_movemask_epi8(x));
            if (r != 0) {   // some of characters may be non-whitespace
#ifdef _MSC_VER         // Find the index of first non-whitespace
                unsigned long offset;
                _BitScanForward(&offset, r);
                return p + offset;
#else
                return p + __builtin_ffs(r) - 1;
#endif
            }
        }

        return SkipWhitespace(p, end);
    }

#elif defined(MERAK_JSON_NEON)

    //! Skip whitespace with ARM Neon instructions, testing 16 8-byte characters at once.
    inline const char *SkipWhitespace_SIMD(const char* p) {
        // Fast return for single non-whitespace
        if (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t')
            ++p;
        else
            return p;

        // 16-byte align to the next boundary
        const char* nextAligned = reinterpret_cast<const char*>((reinterpret_cast<size_t>(p) + 15) & static_cast<size_t>(~15));
        while (p != nextAligned)
            if (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t')
                ++p;
            else
                return p;

        const uint8x16_t w0 = vmovq_n_u8(' ');
        const uint8x16_t w1 = vmovq_n_u8('\n');
        const uint8x16_t w2 = vmovq_n_u8('\r');
        const uint8x16_t w3 = vmovq_n_u8('\t');

        for (;; p += 16) {
            const uint8x16_t s = vld1q_u8(reinterpret_cast<const uint8_t *>(p));
            uint8x16_t x = vceqq_u8(s, w0);
            x = vorrq_u8(x, vceqq_u8(s, w1));
            x = vorrq_u8(x, vceqq_u8(s, w2));
            x = vorrq_u8(x, vceqq_u8(s, w3));

            x = vmvnq_u8(x);                       // Negate
            x = vrev64q_u8(x);                     // Rev in 64
            uint64_t low = vgetq_lane_u64(vreinterpretq_u64_u8(x), 0);   // extract
            uint64_t high = vgetq_lane_u64(vreinterpretq_u64_u8(x), 1);  // extract

            if (low == 0) {
                if (high != 0) {
                    uint32_t lz = internal::clzll(high);
                    return p + 8 + (lz >> 3);
                }
            } else {
                uint32_t lz = internal::clzll(low);
                return p + (lz >> 3);
            }
        }
    }

    inline const char *SkipWhitespace_SIMD(const char* p, const char* end) {
        // Fast return for single non-whitespace
        if (p != end && (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t'))
            ++p;
        else
            return p;

        const uint8x16_t w0 = vmovq_n_u8(' ');
        const uint8x16_t w1 = vmovq_n_u8('\n');
        const uint8x16_t w2 = vmovq_n_u8('\r');
        const uint8x16_t w3 = vmovq_n_u8('\t');

        for (; p <= end - 16; p += 16) {
            const uint8x16_t s = vld1q_u8(reinterpret_cast<const uint8_t *>(p));
            uint8x16_t x = vceqq_u8(s, w0);
            x = vorrq_u8(x, vceqq_u8(s, w1));
            x = vorrq_u8(x, vceqq_u8(s, w2));
            x = vorrq_u8(x, vceqq_u8(s, w3));

            x = vmvnq_u8(x);                       // Negate
            x = vrev64q_u8(x);                     // Rev in 64
            uint64_t low = vgetq_lane_u64(vreinterpretq_u64_u8(x), 0);   // extract
            uint64_t high = vgetq_lane_u64(vreinterpretq_u64_u8(x), 1);  // extract

            if (low == 0) {
                if (high != 0) {
                    uint32_t lz = internal::clzll(high);
                    return p + 8 + (lz >> 3);
                }
            } else {
                uint32_t lz = internal::clzll(low);
                return p + (lz >> 3);
            }
        }

        return SkipWhitespace(p, end);
    }

#endif // MERAK_JSON_NEON

#ifdef MERAK_JSON_SIMD
    //! Template function specialization for InsituStringStream
    template<> inline void SkipWhitespace(InsituStringStream& is) {
        is.src_ = const_cast<char*>(SkipWhitespace_SIMD(is.src_));
    }

    //! Template function specialization for StringStream
    template<> inline void SkipWhitespace(StringStream& is) {
        is.src_ = SkipWhitespace_SIMD(is.src_);
    }

    template<> inline void SkipWhitespace(EncodedInputStream<UTF8<>, MemoryStream>& is) {
        is.is_.src_ = SkipWhitespace_SIMD(is.is_.src_, is.is_.end_);
    }
#endif // MERAK_JSON_SIMD

///////////////////////////////////////////////////////////////////////////////
// GenericReader

//! SAX-style JSON parser. Use \ref Reader for UTF8 encoding and default allocator.
/*! GenericReader parses JSON text from a stream, and send events synchronously to an
    object implementing Handler concept.

    It needs to allocate a stack for storing a single decoded string during
    non-destructive parsing.

    For in-situ parsing, the decoded string is directly written to the source
    text string, no temporary buffer is required.

    A GenericReader object can be reused for parsing multiple JSON text.

    \tparam SourceEncoding Encoding of the input stream.
    \tparam TargetEncoding Encoding of the parse output.
    \tparam StackAllocator Allocator type for stack.
*/
    template<typename SourceEncoding, typename TargetEncoding, typename StackAllocator = CrtAllocator>
    class GenericReader {
    public:
        typedef typename SourceEncoding::char_type char_type; //!< SourceEncoding character type

        //! Constructor.
        /*! \param stackAllocator Optional allocator for allocating stack memory. (Only use for non-destructive parsing)
            \param stackCapacity stack capacity in bytes for storing a single decoded string.  (Only use for non-destructive parsing)
        */
        GenericReader(StackAllocator *stackAllocator = 0, size_t stackCapacity = kDefaultStackCapacity) :
                stack_(stackAllocator, stackCapacity), parseResult_(), state_(IterativeParsingStartState) {}

        //! parse JSON text.
        /*! \tparam parseFlags Combination of \ref ParseFlag.
            \tparam InputStream Type of input stream, implementing Stream concept.
            \tparam Handler Type of handler, implementing Handler concept.
            \param is Input stream to be parsed.
            \param handler The handler to receive events.
            \return Whether the parsing is successful.
        */
        template<unsigned parseFlags, typename InputStream, typename Handler>
        ParseResult parse(InputStream &is, Handler &handler) {
            if (parseFlags & kParseIterativeFlag)
                return IterativeParse<parseFlags>(is, handler);

            parseResult_.clear();

            ClearStackOnExit scope(*this);

            skip_whitespace_and_comments<parseFlags>(is);
            MERAK_JSON_PARSE_ERROR_EARLY_RETURN(parseResult_);

            if (MERAK_JSON_UNLIKELY(is.peek() == '\0')) {
                MERAK_JSON_PARSE_ERROR_NORETURN(kParseErrorDocumentEmpty, is.tellg());
                MERAK_JSON_PARSE_ERROR_EARLY_RETURN(parseResult_);
            } else {
                ParseValue<parseFlags>(is, handler);
                MERAK_JSON_PARSE_ERROR_EARLY_RETURN(parseResult_);

                if (!(parseFlags & kParseStopWhenDoneFlag)) {
                    skip_whitespace_and_comments<parseFlags>(is);
                    MERAK_JSON_PARSE_ERROR_EARLY_RETURN(parseResult_);

                    if (MERAK_JSON_UNLIKELY(is.peek() != '\0')) {
                        MERAK_JSON_PARSE_ERROR_NORETURN(kParseErrorDocumentRootNotSingular, is.tellg());
                        MERAK_JSON_PARSE_ERROR_EARLY_RETURN(parseResult_);
                    }
                } else {
                    // jge: Update parseResult_.Offset() when kParseStopwhendoneflag
                    // is set which means the user needs to know where to resume
                    // parsing in next calls to json_to_proto_message()
                    SetParseError(kParseErrorNone, is.tellg());
                }
            }

            return parseResult_;
        }

        //! parse JSON text (with \ref kParseDefaultFlags)
        /*! \tparam InputStream Type of input stream, implementing Stream concept
            \tparam Handler Type of handler, implementing Handler concept.
            \param is Input stream to be parsed.
            \param handler The handler to receive events.
            \return Whether the parsing is successful.
        */
        template<typename InputStream, typename Handler>
        ParseResult parse(InputStream &is, Handler &handler) {
            return parse<kParseDefaultFlags>(is, handler);
        }

        //! Initialize JSON text token-by-token parsing
        /*!
         */
        void IterativeParseInit() {
            parseResult_.clear();
            state_ = IterativeParsingStartState;
        }

        //! parse one token from JSON text
        /*! \tparam InputStream Type of input stream, implementing Stream concept
            \tparam Handler Type of handler, implementing Handler concept.
            \param is Input stream to be parsed.
            \param handler The handler to receive events.
            \return Whether the parsing is successful.
         */
        template<unsigned parseFlags, typename InputStream, typename Handler>
        bool IterativeParseNext(InputStream &is, Handler &handler) {
            while (MERAK_JSON_LIKELY(is.peek() != '\0')) {
                skip_whitespace_and_comments<parseFlags>(is);

                Token t = Tokenize(is.peek());
                IterativeParsingState n = Predict(state_, t);
                IterativeParsingState d = Transit<parseFlags>(state_, t, n, is, handler);

                // If we've finished or hit an error...
                if (MERAK_JSON_UNLIKELY(IsIterativeParsingCompleteState(d))) {
                    // Report errors.
                    if (d == IterativeParsingErrorState) {
                        HandleError(state_, is);
                        return false;
                    }

                    // Transition to the finish state.
                    MERAK_JSON_ASSERT(d == IterativeParsingFinishState);
                    state_ = d;

                    // If StopWhenDone is not set...
                    if (!(parseFlags & kParseStopWhenDoneFlag)) {
                        // ... and extra non-whitespace data is found...
                        skip_whitespace_and_comments<parseFlags>(is);
                        if (is.peek() != '\0') {
                            // ... this is considered an error.
                            HandleError(state_, is);
                            return false;
                        }
                    }

                    // Success! We are done!
                    return true;
                }

                // Transition to the new state.
                state_ = d;

                // If we parsed anything other than a delimiter, we invoked the handler, so we can return true now.
                if (!IsIterativeParsingDelimiterState(n))
                    return true;
            }

            // We reached the end of file.
            stack_.clear();

            if (state_ != IterativeParsingFinishState) {
                HandleError(state_, is);
                return false;
            }

            return true;
        }

        //! Check if token-by-token parsing JSON text is complete
        /*! \return Whether the JSON has been fully decoded.
         */
        MERAK_JSON_FORCEINLINE bool IterativeParseComplete() const {
            return IsIterativeParsingCompleteState(state_);
        }

        //! Whether a parse error has occurred in the last parsing.
        bool has_parse_error() const { return parseResult_.IsError(); }

        //! Get the \ref ParseErrorCode of last parsing.
        ParseErrorCode GetParseErrorCode() const { return parseResult_.Code(); }

        //! Get the position of last parsing error in input, 0 otherwise.
        size_t get_error_offset() const { return parseResult_.Offset(); }

    protected:
        void SetParseError(ParseErrorCode code, size_t offset) { parseResult_.set(code, offset); }

    private:
        // Prohibit copy constructor & assignment operator.
        GenericReader(const GenericReader &);

        GenericReader &operator=(const GenericReader &);

        void clear_stack() { stack_.clear(); }

        // clear stack on any exit from parse_stream, e.g. due to exception
        struct ClearStackOnExit {
            explicit ClearStackOnExit(GenericReader &r) : r_(r) {}

            ~ClearStackOnExit() { r_.clear_stack(); }

        private:
            GenericReader &r_;

            ClearStackOnExit(const ClearStackOnExit &);

            ClearStackOnExit &operator=(const ClearStackOnExit &);
        };

        template<unsigned parseFlags, typename InputStream>
        void skip_whitespace_and_comments(InputStream &is) {
            SkipWhitespace(is);

            if (parseFlags & kParseCommentsFlag) {
                while (MERAK_JSON_UNLIKELY(Consume(is, '/'))) {
                    if (Consume(is, '*')) {
                        while (true) {
                            if (MERAK_JSON_UNLIKELY(is.peek() == '\0'))
                                MERAK_JSON_PARSE_ERROR(kParseErrorUnspecificSyntaxError, is.tellg());
                            else if (Consume(is, '*')) {
                                if (Consume(is, '/'))
                                    break;
                            } else
                                is.get();
                        }
                    } else if (MERAK_JSON_LIKELY(Consume(is, '/')))
                        while (is.peek() != '\0' && is.get() != '\n') {}
                    else
                        MERAK_JSON_PARSE_ERROR(kParseErrorUnspecificSyntaxError, is.tellg());

                    SkipWhitespace(is);
                }
            }
        }

        // parse object: { string : value, ... }
        template<unsigned parseFlags, typename InputStream, typename Handler>
        void ParseObject(InputStream &is, Handler &handler) {
            MERAK_JSON_ASSERT(is.peek() == '{');
            is.get();  // Skip '{'

            if (MERAK_JSON_UNLIKELY(!handler.start_object()))
                MERAK_JSON_PARSE_ERROR(kParseErrorTermination, is.tellg());

            skip_whitespace_and_comments<parseFlags>(is);
            MERAK_JSON_PARSE_ERROR_EARLY_RETURN_VOID;

            if (Consume(is, '}')) {
                if (MERAK_JSON_UNLIKELY(!handler.end_object(0)))  // empty object
                    MERAK_JSON_PARSE_ERROR(kParseErrorTermination, is.tellg());
                return;
            }

            for (SizeType memberCount = 0;;) {
                if (MERAK_JSON_UNLIKELY(is.peek() != '"'))
                    MERAK_JSON_PARSE_ERROR(kParseErrorObjectMissName, is.tellg());

                ParseString<parseFlags>(is, handler, true);
                MERAK_JSON_PARSE_ERROR_EARLY_RETURN_VOID;

                skip_whitespace_and_comments<parseFlags>(is);
                MERAK_JSON_PARSE_ERROR_EARLY_RETURN_VOID;

                if (MERAK_JSON_UNLIKELY(!Consume(is, ':')))
                    MERAK_JSON_PARSE_ERROR(kParseErrorObjectMissColon, is.tellg());

                skip_whitespace_and_comments<parseFlags>(is);
                MERAK_JSON_PARSE_ERROR_EARLY_RETURN_VOID;

                ParseValue<parseFlags>(is, handler);
                MERAK_JSON_PARSE_ERROR_EARLY_RETURN_VOID;

                skip_whitespace_and_comments<parseFlags>(is);
                MERAK_JSON_PARSE_ERROR_EARLY_RETURN_VOID;

                ++memberCount;

                switch (is.peek()) {
                    case ',':
                        is.get();
                        skip_whitespace_and_comments<parseFlags>(is);
                        MERAK_JSON_PARSE_ERROR_EARLY_RETURN_VOID;
                        break;
                    case '}':
                        is.get();
                        if (MERAK_JSON_UNLIKELY(!handler.end_object(memberCount)))
                            MERAK_JSON_PARSE_ERROR(kParseErrorTermination, is.tellg());
                        return;
                    default:
                        MERAK_JSON_PARSE_ERROR(kParseErrorObjectMissCommaOrCurlyBracket, is.tellg());
                        break; // This useless break is only for making warning and coverage happy
                }

                if (parseFlags & kParseTrailingCommasFlag) {
                    if (is.peek() == '}') {
                        if (MERAK_JSON_UNLIKELY(!handler.end_object(memberCount)))
                            MERAK_JSON_PARSE_ERROR(kParseErrorTermination, is.tellg());
                        is.get();
                        return;
                    }
                }
            }
        }

        // parse array: [ value, ... ]
        template<unsigned parseFlags, typename InputStream, typename Handler>
        void ParseArray(InputStream &is, Handler &handler) {
            MERAK_JSON_ASSERT(is.peek() == '[');
            is.get();  // Skip '['

            if (MERAK_JSON_UNLIKELY(!handler.start_array()))
                MERAK_JSON_PARSE_ERROR(kParseErrorTermination, is.tellg());

            skip_whitespace_and_comments<parseFlags>(is);
            MERAK_JSON_PARSE_ERROR_EARLY_RETURN_VOID;

            if (Consume(is, ']')) {
                if (MERAK_JSON_UNLIKELY(!handler.end_array(0))) // empty array
                    MERAK_JSON_PARSE_ERROR(kParseErrorTermination, is.tellg());
                return;
            }

            for (SizeType elementCount = 0;;) {
                ParseValue<parseFlags>(is, handler);
                MERAK_JSON_PARSE_ERROR_EARLY_RETURN_VOID;

                ++elementCount;
                skip_whitespace_and_comments<parseFlags>(is);
                MERAK_JSON_PARSE_ERROR_EARLY_RETURN_VOID;

                if (Consume(is, ',')) {
                    skip_whitespace_and_comments<parseFlags>(is);
                    MERAK_JSON_PARSE_ERROR_EARLY_RETURN_VOID;
                } else if (Consume(is, ']')) {
                    if (MERAK_JSON_UNLIKELY(!handler.end_array(elementCount)))
                        MERAK_JSON_PARSE_ERROR(kParseErrorTermination, is.tellg());
                    return;
                } else
                    MERAK_JSON_PARSE_ERROR(kParseErrorArrayMissCommaOrSquareBracket, is.tellg());

                if (parseFlags & kParseTrailingCommasFlag) {
                    if (is.peek() == ']') {
                        if (MERAK_JSON_UNLIKELY(!handler.end_array(elementCount)))
                            MERAK_JSON_PARSE_ERROR(kParseErrorTermination, is.tellg());
                        is.get();
                        return;
                    }
                }
            }
        }

        template<unsigned parseFlags, typename InputStream, typename Handler>
        void ParseNull(InputStream &is, Handler &handler) {
            MERAK_JSON_ASSERT(is.peek() == 'n');
            is.get();

            if (MERAK_JSON_LIKELY(Consume(is, 'u') && Consume(is, 'l') && Consume(is, 'l'))) {
                if (MERAK_JSON_UNLIKELY(!handler.emplace_null()))
                    MERAK_JSON_PARSE_ERROR(kParseErrorTermination, is.tellg());
            } else
                MERAK_JSON_PARSE_ERROR(kParseErrorValueInvalid, is.tellg());
        }

        template<unsigned parseFlags, typename InputStream, typename Handler>
        void ParseTrue(InputStream &is, Handler &handler) {
            MERAK_JSON_ASSERT(is.peek() == 't');
            is.get();

            if (MERAK_JSON_LIKELY(Consume(is, 'r') && Consume(is, 'u') && Consume(is, 'e'))) {
                if (MERAK_JSON_UNLIKELY(!handler.emplace_bool(true)))
                    MERAK_JSON_PARSE_ERROR(kParseErrorTermination, is.tellg());
            } else
                MERAK_JSON_PARSE_ERROR(kParseErrorValueInvalid, is.tellg());
        }

        template<unsigned parseFlags, typename InputStream, typename Handler>
        void ParseFalse(InputStream &is, Handler &handler) {
            MERAK_JSON_ASSERT(is.peek() == 'f');
            is.get();

            if (MERAK_JSON_LIKELY(Consume(is, 'a') && Consume(is, 'l') && Consume(is, 's') && Consume(is, 'e'))) {
                if (MERAK_JSON_UNLIKELY(!handler.emplace_bool(false)))
                    MERAK_JSON_PARSE_ERROR(kParseErrorTermination, is.tellg());
            } else
                MERAK_JSON_PARSE_ERROR(kParseErrorValueInvalid, is.tellg());
        }

        template<typename InputStream>
        MERAK_JSON_FORCEINLINE static bool Consume(InputStream &is, typename InputStream::char_type expect) {
            if (MERAK_JSON_LIKELY(is.peek() == expect)) {
                is.get();
                return true;
            } else
                return false;
        }

        // Helper function to parse four hexadecimal digits in \uXXXX in ParseString().
        template<typename InputStream>
        unsigned ParseHex4(InputStream &is, size_t escapeOffset) {
            unsigned codepoint = 0;
            for (int i = 0; i < 4; i++) {
                char_type c = is.peek();
                codepoint <<= 4;
                codepoint += static_cast<unsigned>(c);
                if (c >= '0' && c <= '9')
                    codepoint -= '0';
                else if (c >= 'A' && c <= 'F')
                    codepoint -= 'A' - 10;
                else if (c >= 'a' && c <= 'f')
                    codepoint -= 'a' - 10;
                else {
                    MERAK_JSON_PARSE_ERROR_NORETURN(kParseErrorStringUnicodeEscapeInvalidHex, escapeOffset);
                    MERAK_JSON_PARSE_ERROR_EARLY_RETURN(0);
                }
                is.get();
            }
            return codepoint;
        }

        template<typename CharType>
        class StackStream {
        public:
            typedef CharType char_type;

            StackStream(internal::Stack<StackAllocator> &stack) : stack_(stack), length_(0) {}

            MERAK_JSON_FORCEINLINE void put(char_type c) {
                *stack_.template Push<char_type>() = c;
                ++length_;
            }

            MERAK_JSON_FORCEINLINE void *Push(SizeType count) {
                length_ += count;
                return stack_.template Push<char_type>(count);
            }

            size_t Length() const { return length_; }

            char_type *Pop() {
                return stack_.template Pop<char_type>(length_);
            }

        private:
            StackStream(const StackStream &);

            StackStream &operator=(const StackStream &);

            internal::Stack<StackAllocator> &stack_;
            SizeType length_;
        };

        // parse string and generate String event..
        template<unsigned parseFlags, typename InputStream, typename Handler>
        void ParseString(InputStream &is, Handler &handler, bool isKey = false) {
            internal::StreamLocalCopy<InputStream> copy(is);
            InputStream &s(copy.s);

            MERAK_JSON_ASSERT(s.peek() == '\"');
            s.get();  // Skip '\"'

            bool success = false;

            StackStream<typename TargetEncoding::char_type> stackStream(stack_);
            ParseStringToStream<parseFlags, SourceEncoding, TargetEncoding>(s, stackStream);
            MERAK_JSON_PARSE_ERROR_EARLY_RETURN_VOID;
            SizeType length = static_cast<SizeType>(stackStream.Length()) - 1;
            const typename TargetEncoding::char_type *const str = stackStream.Pop();
            success = (isKey ? handler.emplace_key(str, length, true) : handler.emplace_string(str, length, true));
            if (MERAK_JSON_UNLIKELY(!success))
                MERAK_JSON_PARSE_ERROR(kParseErrorTermination, s.tellg());
        }

        // parse string to an output is
        // This function handles the prefix/suffix double quotes, escaping, and optional encoding validation.
        template<unsigned parseFlags, typename SEncoding, typename TEncoding, typename InputStream, typename OutputStream>
        MERAK_JSON_FORCEINLINE void ParseStringToStream(InputStream &is, OutputStream &os) {
//!@cond MERAK_JSON_HIDDEN_FROM_DOXYGEN
#define Z16 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
            static const char escape[256] = {
                    Z16, Z16, 0, 0, '\"', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '/',
                    Z16, Z16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '\\', 0, 0, 0,
                    0, 0, '\b', 0, 0, 0, '\f', 0, 0, 0, 0, 0, 0, 0, '\n', 0,
                    0, 0, '\r', 0, '\t', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    Z16, Z16, Z16, Z16, Z16, Z16, Z16, Z16
            };
#undef Z16
//!@endcond

            for (;;) {
                // Scan and copy string before "\\\"" or < 0x20. This is an optional optimzation.
                if (!(parseFlags & kParseValidateEncodingFlag))
                    ScanCopyUnescapedString(is, os);

                char_type c = is.peek();
                if (MERAK_JSON_UNLIKELY(c == '\\')) {    // Escape
                    size_t escapeOffset = is.tellg();    // For invalid escaping, report the initial '\\' as error offset
                    is.get();
                    char_type e = is.peek();
                    if ((sizeof(char_type) == 1 || unsigned(e) < 256) &&
                        MERAK_JSON_LIKELY(escape[static_cast<unsigned char>(e)])) {
                        is.get();
                        os.put(static_cast<typename TEncoding::char_type>(escape[static_cast<unsigned char>(e)]));
                    } else if ((parseFlags & kParseEscapedApostropheFlag) &&
                               MERAK_JSON_LIKELY(e == '\'')) { // Allow escaped apostrophe
                        is.get();
                        os.put('\'');
                    } else if (MERAK_JSON_LIKELY(e == 'u')) {    // Unicode
                        is.get();
                        unsigned codepoint = ParseHex4(is, escapeOffset);
                        MERAK_JSON_PARSE_ERROR_EARLY_RETURN_VOID;
                        if (MERAK_JSON_UNLIKELY(codepoint >= 0xD800 && codepoint <= 0xDFFF)) {
                            // high surrogate, check if followed by valid low surrogate
                            if (MERAK_JSON_LIKELY(codepoint <= 0xDBFF)) {
                                // Handle UTF-16 surrogate pair
                                if (MERAK_JSON_UNLIKELY(!Consume(is, '\\') || !Consume(is, 'u')))
                                    MERAK_JSON_PARSE_ERROR(kParseErrorStringUnicodeSurrogateInvalid, escapeOffset);
                                unsigned codepoint2 = ParseHex4(is, escapeOffset);
                                MERAK_JSON_PARSE_ERROR_EARLY_RETURN_VOID;
                                if (MERAK_JSON_UNLIKELY(codepoint2 < 0xDC00 || codepoint2 > 0xDFFF))
                                    MERAK_JSON_PARSE_ERROR(kParseErrorStringUnicodeSurrogateInvalid, escapeOffset);
                                codepoint = (((codepoint - 0xD800) << 10) | (codepoint2 - 0xDC00)) + 0x10000;
                            }
                                // single low surrogate
                            else {
                                MERAK_JSON_PARSE_ERROR(kParseErrorStringUnicodeSurrogateInvalid, escapeOffset);
                            }
                        }
                        TEncoding::Encode(os, codepoint);
                    } else
                        MERAK_JSON_PARSE_ERROR(kParseErrorStringEscapeInvalid, escapeOffset);
                } else if (MERAK_JSON_UNLIKELY(c == '"')) {    // Closing double quote
                    is.get();
                    os.put('\0');   // null-terminate the string
                    return;
                } else if (MERAK_JSON_UNLIKELY(
                        static_cast<unsigned>(c) < 0x20)) { // RFC 4627: unescaped = %x20-21 / %x23-5B / %x5D-10FFFF
                    if (c == '\0')
                        MERAK_JSON_PARSE_ERROR(kParseErrorStringMissQuotationMark, is.tellg());
                    else
                        MERAK_JSON_PARSE_ERROR(kParseErrorStringInvalidEncoding, is.tellg());
                } else {
                    size_t offset = is.tellg();
                    if (MERAK_JSON_UNLIKELY((parseFlags & kParseValidateEncodingFlag ?
                                             !Transcoder<SEncoding, TEncoding>::Validate(is, os) :
                                             !Transcoder<SEncoding, TEncoding>::Transcode(is, os))))
                        MERAK_JSON_PARSE_ERROR(kParseErrorStringInvalidEncoding, offset);
                }
            }
        }

        template<typename InputStream, typename OutputStream>
        static MERAK_JSON_FORCEINLINE void ScanCopyUnescapedString(InputStream &, OutputStream &) {
            // Do nothing for generic version
        }

#if defined(MERAK_JSON_SSE2) || defined(MERAK_JSON_SSE42)
        // StringStream -> StackStream<char>
        static MERAK_JSON_FORCEINLINE void ScanCopyUnescapedString(StringStream& is, StackStream<char>& os) {
            const char* p = is.src_;

            // Scan one by one until alignment (unaligned load may cross page boundary and cause crash)
            const char* nextAligned = reinterpret_cast<const char*>((reinterpret_cast<size_t>(p) + 15) & static_cast<size_t>(~15));
            while (p != nextAligned)
                if (MERAK_JSON_UNLIKELY(*p == '\"') || MERAK_JSON_UNLIKELY(*p == '\\') || MERAK_JSON_UNLIKELY(static_cast<unsigned>(*p) < 0x20)) {
                    is.src_ = p;
                    return;
                }
                else
                    os.put(*p++);

            // The rest of string using SIMD
            static const char dquote[16] = { '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"' };
            static const char bslash[16] = { '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\' };
            static const char space[16]  = { 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F };
            const __m128i dq = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&dquote[0]));
            const __m128i bs = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&bslash[0]));
            const __m128i sp = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&space[0]));

            for (;; p += 16) {
                const __m128i s = _mm_load_si128(reinterpret_cast<const __m128i *>(p));
                const __m128i t1 = _mm_cmpeq_epi8(s, dq);
                const __m128i t2 = _mm_cmpeq_epi8(s, bs);
                const __m128i t3 = _mm_cmpeq_epi8(_mm_max_epu8(s, sp), sp); // s < 0x20 <=> max(s, 0x1F) == 0x1F
                const __m128i x = _mm_or_si128(_mm_or_si128(t1, t2), t3);
                unsigned short r = static_cast<unsigned short>(_mm_movemask_epi8(x));
                if (MERAK_JSON_UNLIKELY(r != 0)) {   // some of characters is escaped
                    SizeType length;
#ifdef _MSC_VER         // Find the index of first escaped
                    unsigned long offset;
                    _BitScanForward(&offset, r);
                    length = offset;
#else
                    length = static_cast<SizeType>(__builtin_ffs(r) - 1);
#endif
                    if (length != 0) {
                        char* q = reinterpret_cast<char*>(os.Push(length));
                        for (size_t i = 0; i < length; i++)
                            q[i] = p[i];

                        p += length;
                    }
                    break;
                }
                _mm_storeu_si128(reinterpret_cast<__m128i *>(os.Push(16)), s);
            }

            is.src_ = p;
        }

        // InsituStringStream -> InsituStringStream
        static MERAK_JSON_FORCEINLINE void ScanCopyUnescapedString(InsituStringStream& is, InsituStringStream& os) {
            MERAK_JSON_ASSERT(&is == &os);
            (void)os;

            if (is.src_ == is.dst_) {
                SkipUnescapedString(is);
                return;
            }

            char* p = is.src_;
            char *q = is.dst_;

            // Scan one by one until alignment (unaligned load may cross page boundary and cause crash)
            const char* nextAligned = reinterpret_cast<const char*>((reinterpret_cast<size_t>(p) + 15) & static_cast<size_t>(~15));
            while (p != nextAligned)
                if (MERAK_JSON_UNLIKELY(*p == '\"') || MERAK_JSON_UNLIKELY(*p == '\\') || MERAK_JSON_UNLIKELY(static_cast<unsigned>(*p) < 0x20)) {
                    is.src_ = p;
                    is.dst_ = q;
                    return;
                }
                else
                    *q++ = *p++;

            // The rest of string using SIMD
            static const char dquote[16] = { '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"' };
            static const char bslash[16] = { '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\' };
            static const char space[16] = { 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F };
            const __m128i dq = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&dquote[0]));
            const __m128i bs = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&bslash[0]));
            const __m128i sp = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&space[0]));

            for (;; p += 16, q += 16) {
                const __m128i s = _mm_load_si128(reinterpret_cast<const __m128i *>(p));
                const __m128i t1 = _mm_cmpeq_epi8(s, dq);
                const __m128i t2 = _mm_cmpeq_epi8(s, bs);
                const __m128i t3 = _mm_cmpeq_epi8(_mm_max_epu8(s, sp), sp); // s < 0x20 <=> max(s, 0x1F) == 0x1F
                const __m128i x = _mm_or_si128(_mm_or_si128(t1, t2), t3);
                unsigned short r = static_cast<unsigned short>(_mm_movemask_epi8(x));
                if (MERAK_JSON_UNLIKELY(r != 0)) {   // some of characters is escaped
                    size_t length;
#ifdef _MSC_VER         // Find the index of first escaped
                    unsigned long offset;
                    _BitScanForward(&offset, r);
                    length = offset;
#else
                    length = static_cast<size_t>(__builtin_ffs(r) - 1);
#endif
                    for (const char* pend = p + length; p != pend; )
                        *q++ = *p++;
                    break;
                }
                _mm_storeu_si128(reinterpret_cast<__m128i *>(q), s);
            }

            is.src_ = p;
            is.dst_ = q;
        }

        // When read/write pointers are the same for insitu stream, just skip unescaped characters
        static MERAK_JSON_FORCEINLINE void SkipUnescapedString(InsituStringStream& is) {
            MERAK_JSON_ASSERT(is.src_ == is.dst_);
            char* p = is.src_;

            // Scan one by one until alignment (unaligned load may cross page boundary and cause crash)
            const char* nextAligned = reinterpret_cast<const char*>((reinterpret_cast<size_t>(p) + 15) & static_cast<size_t>(~15));
            for (; p != nextAligned; p++)
                if (MERAK_JSON_UNLIKELY(*p == '\"') || MERAK_JSON_UNLIKELY(*p == '\\') || MERAK_JSON_UNLIKELY(static_cast<unsigned>(*p) < 0x20)) {
                    is.src_ = is.dst_ = p;
                    return;
                }

            // The rest of string using SIMD
            static const char dquote[16] = { '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"', '\"' };
            static const char bslash[16] = { '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\' };
            static const char space[16] = { 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F };
            const __m128i dq = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&dquote[0]));
            const __m128i bs = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&bslash[0]));
            const __m128i sp = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&space[0]));

            for (;; p += 16) {
                const __m128i s = _mm_load_si128(reinterpret_cast<const __m128i *>(p));
                const __m128i t1 = _mm_cmpeq_epi8(s, dq);
                const __m128i t2 = _mm_cmpeq_epi8(s, bs);
                const __m128i t3 = _mm_cmpeq_epi8(_mm_max_epu8(s, sp), sp); // s < 0x20 <=> max(s, 0x1F) == 0x1F
                const __m128i x = _mm_or_si128(_mm_or_si128(t1, t2), t3);
                unsigned short r = static_cast<unsigned short>(_mm_movemask_epi8(x));
                if (MERAK_JSON_UNLIKELY(r != 0)) {   // some of characters is escaped
                    size_t length;
#ifdef _MSC_VER         // Find the index of first escaped
                    unsigned long offset;
                    _BitScanForward(&offset, r);
                    length = offset;
#else
                    length = static_cast<size_t>(__builtin_ffs(r) - 1);
#endif
                    p += length;
                    break;
                }
            }

            is.src_ = is.dst_ = p;
        }
#elif defined(MERAK_JSON_NEON)
        // StringStream -> StackStream<char>
        static MERAK_JSON_FORCEINLINE void ScanCopyUnescapedString(StringStream& is, StackStream<char>& os) {
            const char* p = is.src_;

            // Scan one by one until alignment (unaligned load may cross page boundary and cause crash)
            const char* nextAligned = reinterpret_cast<const char*>((reinterpret_cast<size_t>(p) + 15) & static_cast<size_t>(~15));
            while (p != nextAligned)
                if (MERAK_JSON_UNLIKELY(*p == '\"') || MERAK_JSON_UNLIKELY(*p == '\\') || MERAK_JSON_UNLIKELY(static_cast<unsigned>(*p) < 0x20)) {
                    is.src_ = p;
                    return;
                }
                else
                    os.put(*p++);

            // The rest of string using SIMD
            const uint8x16_t s0 = vmovq_n_u8('"');
            const uint8x16_t s1 = vmovq_n_u8('\\');
            const uint8x16_t s2 = vmovq_n_u8('\b');
            const uint8x16_t s3 = vmovq_n_u8(32);

            for (;; p += 16) {
                const uint8x16_t s = vld1q_u8(reinterpret_cast<const uint8_t *>(p));
                uint8x16_t x = vceqq_u8(s, s0);
                x = vorrq_u8(x, vceqq_u8(s, s1));
                x = vorrq_u8(x, vceqq_u8(s, s2));
                x = vorrq_u8(x, vcltq_u8(s, s3));

                x = vrev64q_u8(x);                     // Rev in 64
                uint64_t low = vgetq_lane_u64(vreinterpretq_u64_u8(x), 0);   // extract
                uint64_t high = vgetq_lane_u64(vreinterpretq_u64_u8(x), 1);  // extract

                SizeType length = 0;
                bool escaped = false;
                if (low == 0) {
                    if (high != 0) {
                        uint32_t lz = internal::clzll(high);
                        length = 8 + (lz >> 3);
                        escaped = true;
                    }
                } else {
                    uint32_t lz = internal::clzll(low);
                    length = lz >> 3;
                    escaped = true;
                }
                if (MERAK_JSON_UNLIKELY(escaped)) {   // some of characters is escaped
                    if (length != 0) {
                        char* q = reinterpret_cast<char*>(os.Push(length));
                        for (size_t i = 0; i < length; i++)
                            q[i] = p[i];

                        p += length;
                    }
                    break;
                }
                vst1q_u8(reinterpret_cast<uint8_t *>(os.Push(16)), s);
            }

            is.src_ = p;
        }

        // InsituStringStream -> InsituStringStream
        static MERAK_JSON_FORCEINLINE void ScanCopyUnescapedString(InsituStringStream& is, InsituStringStream& os) {
            MERAK_JSON_ASSERT(&is == &os);
            (void)os;

            if (is.src_ == is.dst_) {
                SkipUnescapedString(is);
                return;
            }

            char* p = is.src_;
            char *q = is.dst_;

            // Scan one by one until alignment (unaligned load may cross page boundary and cause crash)
            const char* nextAligned = reinterpret_cast<const char*>((reinterpret_cast<size_t>(p) + 15) & static_cast<size_t>(~15));
            while (p != nextAligned)
                if (MERAK_JSON_UNLIKELY(*p == '\"') || MERAK_JSON_UNLIKELY(*p == '\\') || MERAK_JSON_UNLIKELY(static_cast<unsigned>(*p) < 0x20)) {
                    is.src_ = p;
                    is.dst_ = q;
                    return;
                }
                else
                    *q++ = *p++;

            // The rest of string using SIMD
            const uint8x16_t s0 = vmovq_n_u8('"');
            const uint8x16_t s1 = vmovq_n_u8('\\');
            const uint8x16_t s2 = vmovq_n_u8('\b');
            const uint8x16_t s3 = vmovq_n_u8(32);

            for (;; p += 16, q += 16) {
                const uint8x16_t s = vld1q_u8(reinterpret_cast<uint8_t *>(p));
                uint8x16_t x = vceqq_u8(s, s0);
                x = vorrq_u8(x, vceqq_u8(s, s1));
                x = vorrq_u8(x, vceqq_u8(s, s2));
                x = vorrq_u8(x, vcltq_u8(s, s3));

                x = vrev64q_u8(x);                     // Rev in 64
                uint64_t low = vgetq_lane_u64(vreinterpretq_u64_u8(x), 0);   // extract
                uint64_t high = vgetq_lane_u64(vreinterpretq_u64_u8(x), 1);  // extract

                SizeType length = 0;
                bool escaped = false;
                if (low == 0) {
                    if (high != 0) {
                        uint32_t lz = internal::clzll(high);
                        length = 8 + (lz >> 3);
                        escaped = true;
                    }
                } else {
                    uint32_t lz = internal::clzll(low);
                    length = lz >> 3;
                    escaped = true;
                }
                if (MERAK_JSON_UNLIKELY(escaped)) {   // some of characters is escaped
                    for (const char* pend = p + length; p != pend; ) {
                        *q++ = *p++;
                    }
                    break;
                }
                vst1q_u8(reinterpret_cast<uint8_t *>(q), s);
            }

            is.src_ = p;
            is.dst_ = q;
        }

        // When read/write pointers are the same for insitu stream, just skip unescaped characters
        static MERAK_JSON_FORCEINLINE void SkipUnescapedString(InsituStringStream& is) {
            MERAK_JSON_ASSERT(is.src_ == is.dst_);
            char* p = is.src_;

            // Scan one by one until alignment (unaligned load may cross page boundary and cause crash)
            const char* nextAligned = reinterpret_cast<const char*>((reinterpret_cast<size_t>(p) + 15) & static_cast<size_t>(~15));
            for (; p != nextAligned; p++)
                if (MERAK_JSON_UNLIKELY(*p == '\"') || MERAK_JSON_UNLIKELY(*p == '\\') || MERAK_JSON_UNLIKELY(static_cast<unsigned>(*p) < 0x20)) {
                    is.src_ = is.dst_ = p;
                    return;
                }

            // The rest of string using SIMD
            const uint8x16_t s0 = vmovq_n_u8('"');
            const uint8x16_t s1 = vmovq_n_u8('\\');
            const uint8x16_t s2 = vmovq_n_u8('\b');
            const uint8x16_t s3 = vmovq_n_u8(32);

            for (;; p += 16) {
                const uint8x16_t s = vld1q_u8(reinterpret_cast<uint8_t *>(p));
                uint8x16_t x = vceqq_u8(s, s0);
                x = vorrq_u8(x, vceqq_u8(s, s1));
                x = vorrq_u8(x, vceqq_u8(s, s2));
                x = vorrq_u8(x, vcltq_u8(s, s3));

                x = vrev64q_u8(x);                     // Rev in 64
                uint64_t low = vgetq_lane_u64(vreinterpretq_u64_u8(x), 0);   // extract
                uint64_t high = vgetq_lane_u64(vreinterpretq_u64_u8(x), 1);  // extract

                if (low == 0) {
                    if (high != 0) {
                        uint32_t lz = internal::clzll(high);
                        p += 8 + (lz >> 3);
                        break;
                    }
                } else {
                    uint32_t lz = internal::clzll(low);
                    p += lz >> 3;
                    break;
                }
            }

            is.src_ = is.dst_ = p;
        }
#endif // MERAK_JSON_NEON

        template<typename InputStream, typename StackCharacter, bool backup, bool pushOnTake>
        class NumberStream;

        template<typename InputStream, typename StackCharacter>
        class NumberStream<InputStream, StackCharacter, false, false> {
        public:
            typedef typename InputStream::char_type char_type;

            NumberStream(GenericReader &reader, InputStream &s) : is(s) { (void) reader; }

            MERAK_JSON_FORCEINLINE char_type peek() const { return is.peek(); }

            MERAK_JSON_FORCEINLINE char_type TakePush() { return is.get(); }

            MERAK_JSON_FORCEINLINE char_type get() { return is.get(); }

            MERAK_JSON_FORCEINLINE void Push(char) {}

            size_t tellg() { return is.tellg(); }

            size_t Length() { return 0; }

            const StackCharacter *Pop() { return 0; }

        protected:
            NumberStream &operator=(const NumberStream &);

            InputStream &is;
        };

        template<typename InputStream, typename StackCharacter>
        class NumberStream<InputStream, StackCharacter, true, false>
                : public NumberStream<InputStream, StackCharacter, false, false> {
            typedef NumberStream<InputStream, StackCharacter, false, false> Base;
        public:
            NumberStream(GenericReader &reader, InputStream &s) : Base(reader, s), stackStream(reader.stack_) {}

            MERAK_JSON_FORCEINLINE char_type TakePush() {
                stackStream.put(static_cast<StackCharacter>(Base::is.peek()));
                return Base::is.get();
            }

            MERAK_JSON_FORCEINLINE void Push(StackCharacter c) {
                stackStream.put(c);
            }

            size_t Length() { return stackStream.Length(); }

            const StackCharacter *Pop() {
                stackStream.put('\0');
                return stackStream.Pop();
            }

        private:
            StackStream<StackCharacter> stackStream;
        };

        template<typename InputStream, typename StackCharacter>
        class NumberStream<InputStream, StackCharacter, true, true>
                : public NumberStream<InputStream, StackCharacter, true, false> {
            typedef NumberStream<InputStream, StackCharacter, true, false> Base;
        public:
            NumberStream(GenericReader &reader, InputStream &s) : Base(reader, s) {}

            MERAK_JSON_FORCEINLINE char_type get() { return Base::TakePush(); }
        };

        template<unsigned parseFlags, typename InputStream, typename Handler>
        void ParseNumber(InputStream &is, Handler &handler) {
            typedef typename internal::SelectIf<internal::BoolType<(parseFlags & kParseNumbersAsStringsFlag) !=
                                                                   0>, typename TargetEncoding::char_type, char>::Type NumberCharacter;

            internal::StreamLocalCopy<InputStream> copy(is);
            NumberStream<InputStream, NumberCharacter,
                    ((parseFlags & kParseNumbersAsStringsFlag) != 0) ?
                    true :
                    ((parseFlags & kParseFullPrecisionFlag) != 0),
                    (parseFlags & kParseNumbersAsStringsFlag) != 0> s(*this, copy.s);

            size_t startOffset = s.tellg();
            double d = 0.0;
            bool useNanOrInf = false;

            // parse minus
            bool minus = Consume(s, '-');

            // parse int: zero / ( digit1-9 *DIGIT )
            unsigned i = 0;
            uint64_t i64 = 0;
            bool use64bit = false;
            int significandDigit = 0;
            if (MERAK_JSON_UNLIKELY(s.peek() == '0')) {
                i = 0;
                s.TakePush();
            } else if (MERAK_JSON_LIKELY(s.peek() >= '1' && s.peek() <= '9')) {
                i = static_cast<unsigned>(s.TakePush() - '0');

                if (minus)
                    while (MERAK_JSON_LIKELY(s.peek() >= '0' && s.peek() <= '9')) {
                        if (MERAK_JSON_UNLIKELY(i >= 214748364)) { // 2^31 = 2147483648
                            if (MERAK_JSON_LIKELY(i != 214748364 || s.peek() > '8')) {
                                i64 = i;
                                use64bit = true;
                                break;
                            }
                        }
                        i = i * 10 + static_cast<unsigned>(s.TakePush() - '0');
                        significandDigit++;
                    }
                else
                    while (MERAK_JSON_LIKELY(s.peek() >= '0' && s.peek() <= '9')) {
                        if (MERAK_JSON_UNLIKELY(i >= 429496729)) { // 2^32 - 1 = 4294967295
                            if (MERAK_JSON_LIKELY(i != 429496729 || s.peek() > '5')) {
                                i64 = i;
                                use64bit = true;
                                break;
                            }
                        }
                        i = i * 10 + static_cast<unsigned>(s.TakePush() - '0');
                        significandDigit++;
                    }
            }
                // parse NaN or Infinity here
            else if ((parseFlags & kParseNanAndInfFlag) && MERAK_JSON_LIKELY((s.peek() == 'I' || s.peek() == 'N'))) {
                if (Consume(s, 'N')) {
                    if (Consume(s, 'a') && Consume(s, 'N')) {
                        d = std::numeric_limits<double>::quiet_NaN();
                        useNanOrInf = true;
                    }
                } else if (MERAK_JSON_LIKELY(Consume(s, 'I'))) {
                    if (Consume(s, 'n') && Consume(s, 'f')) {
                        d = (minus ? -std::numeric_limits<double>::infinity()
                                   : std::numeric_limits<double>::infinity());
                        useNanOrInf = true;

                        if (MERAK_JSON_UNLIKELY(s.peek() == 'i' && !(Consume(s, 'i') && Consume(s, 'n')
                                                                     && Consume(s, 'i') && Consume(s, 't') &&
                                                                     Consume(s, 'y')))) {
                            MERAK_JSON_PARSE_ERROR(kParseErrorValueInvalid, s.tellg());
                        }
                    }
                }

                if (MERAK_JSON_UNLIKELY(!useNanOrInf)) {
                    MERAK_JSON_PARSE_ERROR(kParseErrorValueInvalid, s.tellg());
                }
            } else
                MERAK_JSON_PARSE_ERROR(kParseErrorValueInvalid, s.tellg());

            // parse 64bit int
            bool useDouble = false;
            if (use64bit) {
                if (minus)
                    while (MERAK_JSON_LIKELY(s.peek() >= '0' && s.peek() <= '9')) {
                        if (MERAK_JSON_UNLIKELY(
                                i64 >= MERAK_JSON_UINT64_C2(0x0CCCCCCC, 0xCCCCCCCC))) // 2^63 = 9223372036854775808
                            if (MERAK_JSON_LIKELY(
                                    i64 != MERAK_JSON_UINT64_C2(0x0CCCCCCC, 0xCCCCCCCC) || s.peek() > '8')) {
                                d = static_cast<double>(i64);
                                useDouble = true;
                                break;
                            }
                        i64 = i64 * 10 + static_cast<unsigned>(s.TakePush() - '0');
                        significandDigit++;
                    }
                else
                    while (MERAK_JSON_LIKELY(s.peek() >= '0' && s.peek() <= '9')) {
                        if (MERAK_JSON_UNLIKELY(
                                i64 >= MERAK_JSON_UINT64_C2(0x19999999, 0x99999999))) // 2^64 - 1 = 18446744073709551615
                            if (MERAK_JSON_LIKELY(
                                    i64 != MERAK_JSON_UINT64_C2(0x19999999, 0x99999999) || s.peek() > '5')) {
                                d = static_cast<double>(i64);
                                useDouble = true;
                                break;
                            }
                        i64 = i64 * 10 + static_cast<unsigned>(s.TakePush() - '0');
                        significandDigit++;
                    }
            }

            // Force double for big integer
            if (useDouble) {
                while (MERAK_JSON_LIKELY(s.peek() >= '0' && s.peek() <= '9')) {
                    d = d * 10 + (s.TakePush() - '0');
                }
            }

            // parse frac = decimal-point 1*DIGIT
            int expFrac = 0;
            size_t decimalPosition;
            if (!useNanOrInf && Consume(s, '.')) {
                decimalPosition = s.Length();

                if (MERAK_JSON_UNLIKELY(!(s.peek() >= '0' && s.peek() <= '9')))
                    MERAK_JSON_PARSE_ERROR(kParseErrorNumberMissFraction, s.tellg());

                if (!useDouble) {
#if MERAK_JSON_64BIT
                    // Use i64 to store significand in 64-bit architecture
                    if (!use64bit)
                        i64 = i;

                    while (MERAK_JSON_LIKELY(s.peek() >= '0' && s.peek() <= '9')) {
                        if (i64 > MERAK_JSON_UINT64_C2(0x1FFFFF, 0xFFFFFFFF)) // 2^53 - 1 for fast path
                            break;
                        else {
                            i64 = i64 * 10 + static_cast<unsigned>(s.TakePush() - '0');
                            --expFrac;
                            if (i64 != 0)
                                significandDigit++;
                        }
                    }

                    d = static_cast<double>(i64);
#else
                    // Use double to store significand in 32-bit architecture
                    d = static_cast<double>(use64bit ? i64 : i);
#endif
                    useDouble = true;
                }

                while (MERAK_JSON_LIKELY(s.peek() >= '0' && s.peek() <= '9')) {
                    if (significandDigit < 17) {
                        d = d * 10.0 + (s.TakePush() - '0');
                        --expFrac;
                        if (MERAK_JSON_LIKELY(d > 0.0))
                            significandDigit++;
                    } else
                        s.TakePush();
                }
            } else
                decimalPosition = s.Length(); // decimal position at the end of integer.

            // parse exp = e [ minus / plus ] 1*DIGIT
            int exp = 0;
            if (!useNanOrInf && (Consume(s, 'e') || Consume(s, 'E'))) {
                if (!useDouble) {
                    d = static_cast<double>(use64bit ? i64 : i);
                    useDouble = true;
                }

                bool expMinus = false;
                if (Consume(s, '+'));
                else if (Consume(s, '-'))
                    expMinus = true;

                if (MERAK_JSON_LIKELY(s.peek() >= '0' && s.peek() <= '9')) {
                    exp = static_cast<int>(s.get() - '0');
                    if (expMinus) {
                        // (exp + expFrac) must not underflow int => we're detecting when -exp gets
                        // dangerously close to INT_MIN (a pessimistic next digit 9 would push it into
                        // underflow territory):
                        //
                        //        -(exp * 10 + 9) + expFrac >= INT_MIN
                        //   <=>  exp <= (expFrac - INT_MIN - 9) / 10
                        MERAK_JSON_ASSERT(expFrac <= 0);
                        int maxExp = (expFrac + 2147483639) / 10;

                        while (MERAK_JSON_LIKELY(s.peek() >= '0' && s.peek() <= '9')) {
                            exp = exp * 10 + static_cast<int>(s.get() - '0');
                            if (MERAK_JSON_UNLIKELY(exp > maxExp)) {
                                while (MERAK_JSON_UNLIKELY(
                                        s.peek() >= '0' && s.peek() <= '9'))  // Consume the rest of exponent
                                    s.get();
                            }
                        }
                    } else {  // positive exp
                        int maxExp = 308 - expFrac;
                        while (MERAK_JSON_LIKELY(s.peek() >= '0' && s.peek() <= '9')) {
                            exp = exp * 10 + static_cast<int>(s.get() - '0');
                            if (MERAK_JSON_UNLIKELY(exp > maxExp))
                                MERAK_JSON_PARSE_ERROR(kParseErrorNumberTooBig, startOffset);
                        }
                    }
                } else
                    MERAK_JSON_PARSE_ERROR(kParseErrorNumberMissExponent, s.tellg());

                if (expMinus)
                    exp = -exp;
            }

            // Finish parsing, call event according to the type of number.
            bool cont = true;

            if (parseFlags & kParseNumbersAsStringsFlag) {
                SizeType numCharsToCopy = static_cast<SizeType>(s.Length());
                GenericStringStream<UTF8<NumberCharacter> > srcStream(s.Pop());
                StackStream<typename TargetEncoding::char_type> dstStream(stack_);
                while (numCharsToCopy--) {
                    Transcoder<UTF8<typename TargetEncoding::char_type>, TargetEncoding>::Transcode(srcStream, dstStream);
                }
                dstStream.put('\0');
                const typename TargetEncoding::char_type *str = dstStream.Pop();
                const SizeType length = static_cast<SizeType>(dstStream.Length()) - 1;
                cont = handler.raw_number(str, SizeType(length), true);

            } else {
                size_t length = s.Length();
                const NumberCharacter *decimal = s.Pop();  // Pop stack no matter if it will be used or not.

                if (useDouble) {
                    int p = exp + expFrac;
                    if (parseFlags & kParseFullPrecisionFlag)
                        d = internal::StrtodFullPrecision(d, p, decimal, length, decimalPosition, exp);
                    else
                        d = internal::StrtodNormalPrecision(d, p);

                    // Use > max, instead of == inf, to fix bogus warning -Wfloat-equal
                    if (d > (std::numeric_limits<double>::max)()) {
                        // Overflow
                        // TODO: internal::StrtodX should report overflow (or underflow)
                        MERAK_JSON_PARSE_ERROR(kParseErrorNumberTooBig, startOffset);
                    }

                    cont = handler.emplace_double(minus ? -d : d);
                } else if (useNanOrInf) {
                    cont = handler.emplace_double(d);
                } else {
                    if (use64bit) {
                        if (minus)
                            cont = handler.emplace_int64(static_cast<int64_t>(~i64 + 1));
                        else
                            cont = handler.emplace_uint64(i64);
                    } else {
                        if (minus)
                            cont = handler.emplace_int32(static_cast<int32_t>(~i + 1));
                        else
                            cont = handler.emplace_uint32(i);
                    }
                }
            }
            if (MERAK_JSON_UNLIKELY(!cont))
                MERAK_JSON_PARSE_ERROR(kParseErrorTermination, startOffset);
        }

        // parse any JSON value
        template<unsigned parseFlags, typename InputStream, typename Handler>
        void ParseValue(InputStream &is, Handler &handler) {
            switch (is.peek()) {
                case 'n':
                    ParseNull<parseFlags>(is, handler);
                    break;
                case 't':
                    ParseTrue<parseFlags>(is, handler);
                    break;
                case 'f':
                    ParseFalse<parseFlags>(is, handler);
                    break;
                case '"':
                    ParseString<parseFlags>(is, handler);
                    break;
                case '{':
                    ParseObject<parseFlags>(is, handler);
                    break;
                case '[':
                    ParseArray<parseFlags>(is, handler);
                    break;
                default :
                    ParseNumber<parseFlags>(is, handler);
                    break;

            }
        }

        // Iterative Parsing

        // States
        enum IterativeParsingState {
            IterativeParsingFinishState = 0, // sink states at top
            IterativeParsingErrorState,      // sink states at top
            IterativeParsingStartState,

            // Object states
            IterativeParsingObjectInitialState,
            IterativeParsingMemberKeyState,
            IterativeParsingMemberValueState,
            IterativeParsingObjectFinishState,

            // Array states
            IterativeParsingArrayInitialState,
            IterativeParsingElementState,
            IterativeParsingArrayFinishState,

            // Single value state
            IterativeParsingValueState,

            // Delimiter states (at bottom)
            IterativeParsingElementDelimiterState,
            IterativeParsingMemberDelimiterState,
            IterativeParsingKeyValueDelimiterState,

            cIterativeParsingStateCount
        };

        // Tokens
        enum Token {
            LeftBracketToken = 0,
            RightBracketToken,

            LeftCurlyBracketToken,
            RightCurlyBracketToken,

            CommaToken,
            ColonToken,

            StringToken,
            FalseToken,
            TrueToken,
            NullToken,
            NumberToken,

            kTokenCount
        };

        MERAK_JSON_FORCEINLINE Token Tokenize(char_type c) const {

//!@cond MERAK_JSON_HIDDEN_FROM_DOXYGEN
#define N NumberToken
#define N16 N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N
            // Maps from ASCII to Token
            static const unsigned char tokenMap[256] = {
                    N16, // 00~0F
                    N16, // 10~1F
                    N, N, StringToken, N, N, N, N, N, N, N, N, N, CommaToken, N, N, N, // 20~2F
                    N, N, N, N, N, N, N, N, N, N, ColonToken, N, N, N, N, N, // 30~3F
                    N16, // 40~4F
                    N, N, N, N, N, N, N, N, N, N, N, LeftBracketToken, N, RightBracketToken, N, N, // 50~5F
                    N, N, N, N, N, N, FalseToken, N, N, N, N, N, N, N, NullToken, N, // 60~6F
                    N, N, N, N, TrueToken, N, N, N, N, N, N, LeftCurlyBracketToken, N, RightCurlyBracketToken, N,
                    N, // 70~7F
                    N16, N16, N16, N16, N16, N16, N16, N16 // 80~FF
            };
#undef N
#undef N16
//!@endcond

            if (sizeof(char_type) == 1 || static_cast<unsigned>(c) < 256)
                return static_cast<Token>(tokenMap[static_cast<unsigned char>(c)]);
            else
                return NumberToken;
        }

        MERAK_JSON_FORCEINLINE IterativeParsingState Predict(IterativeParsingState state, Token token) const {
            // current state x one lookahead token -> new state
            static const char G[cIterativeParsingStateCount][kTokenCount] = {
                    // Finish(sink state)
                    {
                            IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState,
                            IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState,
                            IterativeParsingErrorState
                    },
                    // Error(sink state)
                    {
                            IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState,
                            IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState,
                            IterativeParsingErrorState
                    },
                    // Start
                    {
                            IterativeParsingArrayInitialState,  // Left bracket
                            IterativeParsingErrorState,         // Right bracket
                            IterativeParsingObjectInitialState, // Left curly bracket
                            IterativeParsingErrorState,         // Right curly bracket
                            IterativeParsingErrorState,         // Comma
                            IterativeParsingErrorState,         // Colon
                            IterativeParsingValueState,         // String
                            IterativeParsingValueState,         // False
                            IterativeParsingValueState,         // True
                            IterativeParsingValueState,         // Null
                            IterativeParsingValueState          // Number
                    },
                    // ObjectInitial
                    {
                            IterativeParsingErrorState,         // Left bracket
                            IterativeParsingErrorState,         // Right bracket
                            IterativeParsingErrorState,         // Left curly bracket
                            IterativeParsingObjectFinishState,  // Right curly bracket
                            IterativeParsingErrorState,         // Comma
                            IterativeParsingErrorState,         // Colon
                            IterativeParsingMemberKeyState,     // String
                            IterativeParsingErrorState,         // False
                            IterativeParsingErrorState,         // True
                            IterativeParsingErrorState,         // Null
                            IterativeParsingErrorState          // Number
                    },
                    // MemberKey
                    {
                            IterativeParsingErrorState,             // Left bracket
                            IterativeParsingErrorState,             // Right bracket
                            IterativeParsingErrorState,             // Left curly bracket
                            IterativeParsingErrorState,             // Right curly bracket
                            IterativeParsingErrorState,             // Comma
                            IterativeParsingKeyValueDelimiterState, // Colon
                            IterativeParsingErrorState,             // String
                            IterativeParsingErrorState,             // False
                            IterativeParsingErrorState,             // True
                            IterativeParsingErrorState,             // Null
                            IterativeParsingErrorState              // Number
                    },
                    // MemberValue
                    {
                            IterativeParsingErrorState,             // Left bracket
                            IterativeParsingErrorState,             // Right bracket
                            IterativeParsingErrorState,             // Left curly bracket
                            IterativeParsingObjectFinishState,      // Right curly bracket
                            IterativeParsingMemberDelimiterState,   // Comma
                            IterativeParsingErrorState,             // Colon
                            IterativeParsingErrorState,             // String
                            IterativeParsingErrorState,             // False
                            IterativeParsingErrorState,             // True
                            IterativeParsingErrorState,             // Null
                            IterativeParsingErrorState              // Number
                    },
                    // ObjectFinish(sink state)
                    {
                            IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState,
                            IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState,
                            IterativeParsingErrorState
                    },
                    // ArrayInitial
                    {
                            IterativeParsingArrayInitialState,      // Left bracket(push Element state)
                            IterativeParsingArrayFinishState,       // Right bracket
                            IterativeParsingObjectInitialState,     // Left curly bracket(push Element state)
                            IterativeParsingErrorState,             // Right curly bracket
                            IterativeParsingErrorState,             // Comma
                            IterativeParsingErrorState,             // Colon
                            IterativeParsingElementState,           // String
                            IterativeParsingElementState,           // False
                            IterativeParsingElementState,           // True
                            IterativeParsingElementState,           // Null
                            IterativeParsingElementState            // Number
                    },
                    // Element
                    {
                            IterativeParsingErrorState,             // Left bracket
                            IterativeParsingArrayFinishState,       // Right bracket
                            IterativeParsingErrorState,             // Left curly bracket
                            IterativeParsingErrorState,             // Right curly bracket
                            IterativeParsingElementDelimiterState,  // Comma
                            IterativeParsingErrorState,             // Colon
                            IterativeParsingErrorState,             // String
                            IterativeParsingErrorState,             // False
                            IterativeParsingErrorState,             // True
                            IterativeParsingErrorState,             // Null
                            IterativeParsingErrorState              // Number
                    },
                    // ArrayFinish(sink state)
                    {
                            IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState,
                            IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState,
                            IterativeParsingErrorState
                    },
                    // Single Value (sink state)
                    {
                            IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState,
                            IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState, IterativeParsingErrorState,
                            IterativeParsingErrorState
                    },
                    // ElementDelimiter
                    {
                            IterativeParsingArrayInitialState,      // Left bracket(push Element state)
                            IterativeParsingArrayFinishState,       // Right bracket
                            IterativeParsingObjectInitialState,     // Left curly bracket(push Element state)
                            IterativeParsingErrorState,             // Right curly bracket
                            IterativeParsingErrorState,             // Comma
                            IterativeParsingErrorState,             // Colon
                            IterativeParsingElementState,           // String
                            IterativeParsingElementState,           // False
                            IterativeParsingElementState,           // True
                            IterativeParsingElementState,           // Null
                            IterativeParsingElementState            // Number
                    },
                    // MemberDelimiter
                    {
                            IterativeParsingErrorState,         // Left bracket
                            IterativeParsingErrorState,         // Right bracket
                            IterativeParsingErrorState,         // Left curly bracket
                            IterativeParsingObjectFinishState,  // Right curly bracket
                            IterativeParsingErrorState,         // Comma
                            IterativeParsingErrorState,         // Colon
                            IterativeParsingMemberKeyState,     // String
                            IterativeParsingErrorState,         // False
                            IterativeParsingErrorState,         // True
                            IterativeParsingErrorState,         // Null
                            IterativeParsingErrorState          // Number
                    },
                    // KeyValueDelimiter
                    {
                            IterativeParsingArrayInitialState,      // Left bracket(push MemberValue state)
                            IterativeParsingErrorState,             // Right bracket
                            IterativeParsingObjectInitialState,     // Left curly bracket(push MemberValue state)
                            IterativeParsingErrorState,             // Right curly bracket
                            IterativeParsingErrorState,             // Comma
                            IterativeParsingErrorState,             // Colon
                            IterativeParsingMemberValueState,       // String
                            IterativeParsingMemberValueState,       // False
                            IterativeParsingMemberValueState,       // True
                            IterativeParsingMemberValueState,       // Null
                            IterativeParsingMemberValueState        // Number
                    },
            }; // end of G

            return static_cast<IterativeParsingState>(G[state][token]);
        }

        // Make an advance in the token stream and state based on the candidate destination state which was returned by Transit().
        // May return a new state on state pop.
        template<unsigned parseFlags, typename InputStream, typename Handler>
        MERAK_JSON_FORCEINLINE IterativeParsingState
        Transit(IterativeParsingState src, Token token, IterativeParsingState dst, InputStream &is, Handler &handler) {
            (void) token;

            switch (dst) {
                case IterativeParsingErrorState:
                    return dst;

                case IterativeParsingObjectInitialState:
                case IterativeParsingArrayInitialState: {
                    // Push the state(Element or MemeberValue) if we are nested in another array or value of member.
                    // In this way we can get the correct state on ObjectFinish or ArrayFinish by frame pop.
                    IterativeParsingState n = src;
                    if (src == IterativeParsingArrayInitialState || src == IterativeParsingElementDelimiterState)
                        n = IterativeParsingElementState;
                    else if (src == IterativeParsingKeyValueDelimiterState)
                        n = IterativeParsingMemberValueState;
                    // Push current state.
                    *stack_.template Push<SizeType>(1) = n;
                    // Initialize and push the member/element count.
                    *stack_.template Push<SizeType>(1) = 0;
                    // Call handler
                    bool hr = (dst == IterativeParsingObjectInitialState) ? handler.start_object()
                                                                          : handler.start_array();
                    // On handler short circuits the parsing.
                    if (!hr) {
                        MERAK_JSON_PARSE_ERROR_NORETURN(kParseErrorTermination, is.tellg());
                        return IterativeParsingErrorState;
                    } else {
                        is.get();
                        return dst;
                    }
                }

                case IterativeParsingMemberKeyState:
                    ParseString<parseFlags>(is, handler, true);
                    if (has_parse_error())
                        return IterativeParsingErrorState;
                    else
                        return dst;

                case IterativeParsingKeyValueDelimiterState:
                    MERAK_JSON_ASSERT(token == ColonToken);
                    is.get();
                    return dst;

                case IterativeParsingMemberValueState:
                    // Must be non-compound value. Or it would be ObjectInitial or ArrayInitial state.
                    ParseValue<parseFlags>(is, handler);
                    if (has_parse_error()) {
                        return IterativeParsingErrorState;
                    }
                    return dst;

                case IterativeParsingElementState:
                    // Must be non-compound value. Or it would be ObjectInitial or ArrayInitial state.
                    ParseValue<parseFlags>(is, handler);
                    if (has_parse_error()) {
                        return IterativeParsingErrorState;
                    }
                    return dst;

                case IterativeParsingMemberDelimiterState:
                case IterativeParsingElementDelimiterState:
                    is.get();
                    // Update member/element count.
                    *stack_.template Top<SizeType>() = *stack_.template Top<SizeType>() + 1;
                    return dst;

                case IterativeParsingObjectFinishState: {
                    // Transit from delimiter is only allowed when trailing commas are enabled
                    if (!(parseFlags & kParseTrailingCommasFlag) && src == IterativeParsingMemberDelimiterState) {
                        MERAK_JSON_PARSE_ERROR_NORETURN(kParseErrorObjectMissName, is.tellg());
                        return IterativeParsingErrorState;
                    }
                    // Get member count.
                    SizeType c = *stack_.template Pop<SizeType>(1);
                    // If the object is not empty, count the last member.
                    if (src == IterativeParsingMemberValueState)
                        ++c;
                    // Restore the state.
                    IterativeParsingState n = static_cast<IterativeParsingState>(*stack_.template Pop<SizeType>(1));
                    // Transit to Finish state if this is the topmost scope.
                    if (n == IterativeParsingStartState)
                        n = IterativeParsingFinishState;
                    // Call handler
                    bool hr = handler.end_object(c);
                    // On handler short circuits the parsing.
                    if (!hr) {
                        MERAK_JSON_PARSE_ERROR_NORETURN(kParseErrorTermination, is.tellg());
                        return IterativeParsingErrorState;
                    } else {
                        is.get();
                        return n;
                    }
                }

                case IterativeParsingArrayFinishState: {
                    // Transit from delimiter is only allowed when trailing commas are enabled
                    if (!(parseFlags & kParseTrailingCommasFlag) && src == IterativeParsingElementDelimiterState) {
                        MERAK_JSON_PARSE_ERROR_NORETURN(kParseErrorValueInvalid, is.tellg());
                        return IterativeParsingErrorState;
                    }
                    // Get element count.
                    SizeType c = *stack_.template Pop<SizeType>(1);
                    // If the array is not empty, count the last element.
                    if (src == IterativeParsingElementState)
                        ++c;
                    // Restore the state.
                    IterativeParsingState n = static_cast<IterativeParsingState>(*stack_.template Pop<SizeType>(1));
                    // Transit to Finish state if this is the topmost scope.
                    if (n == IterativeParsingStartState)
                        n = IterativeParsingFinishState;
                    // Call handler
                    bool hr = handler.end_array(c);
                    // On handler short circuits the parsing.
                    if (!hr) {
                        MERAK_JSON_PARSE_ERROR_NORETURN(kParseErrorTermination, is.tellg());
                        return IterativeParsingErrorState;
                    } else {
                        is.get();
                        return n;
                    }
                }

                default:
                    // This branch is for IterativeParsingValueState actually.
                    // Use `default:` rather than
                    // `case IterativeParsingValueState:` is for code coverage.

                    // The IterativeParsingStartState is not enumerated in this switch-case.
                    // It is impossible for that case. And it can be caught by following assertion.

                    // The IterativeParsingFinishState is not enumerated in this switch-case either.
                    // It is a "derivative" state which cannot triggered from Predict() directly.
                    // Therefore it cannot happen here. And it can be caught by following assertion.
                    MERAK_JSON_ASSERT(dst == IterativeParsingValueState);

                    // Must be non-compound value. Or it would be ObjectInitial or ArrayInitial state.
                    ParseValue<parseFlags>(is, handler);
                    if (has_parse_error()) {
                        return IterativeParsingErrorState;
                    }
                    return IterativeParsingFinishState;
            }
        }

        template<typename InputStream>
        void HandleError(IterativeParsingState src, InputStream &is) {
            if (has_parse_error()) {
                // Error flag has been set.
                return;
            }

            switch (src) {
                case IterativeParsingStartState:
                    MERAK_JSON_PARSE_ERROR(kParseErrorDocumentEmpty, is.tellg());
                    return;
                case IterativeParsingFinishState:
                    MERAK_JSON_PARSE_ERROR(kParseErrorDocumentRootNotSingular, is.tellg());
                    return;
                case IterativeParsingObjectInitialState:
                case IterativeParsingMemberDelimiterState:
                    MERAK_JSON_PARSE_ERROR(kParseErrorObjectMissName, is.tellg());
                    return;
                case IterativeParsingMemberKeyState:
                    MERAK_JSON_PARSE_ERROR(kParseErrorObjectMissColon, is.tellg());
                    return;
                case IterativeParsingMemberValueState:
                    MERAK_JSON_PARSE_ERROR(kParseErrorObjectMissCommaOrCurlyBracket, is.tellg());
                    return;
                case IterativeParsingKeyValueDelimiterState:
                case IterativeParsingArrayInitialState:
                case IterativeParsingElementDelimiterState:
                    MERAK_JSON_PARSE_ERROR(kParseErrorValueInvalid, is.tellg());
                    return;
                default:
                    MERAK_JSON_ASSERT(src == IterativeParsingElementState);
                    MERAK_JSON_PARSE_ERROR(kParseErrorArrayMissCommaOrSquareBracket, is.tellg());
                    return;
            }
        }

        MERAK_JSON_FORCEINLINE bool IsIterativeParsingDelimiterState(IterativeParsingState s) const {
            return s >= IterativeParsingElementDelimiterState;
        }

        MERAK_JSON_FORCEINLINE bool IsIterativeParsingCompleteState(IterativeParsingState s) const {
            return s <= IterativeParsingErrorState;
        }

        template<unsigned parseFlags, typename InputStream, typename Handler>
        ParseResult IterativeParse(InputStream &is, Handler &handler) {
            parseResult_.clear();
            ClearStackOnExit scope(*this);
            IterativeParsingState state = IterativeParsingStartState;

            skip_whitespace_and_comments<parseFlags>(is);
            MERAK_JSON_PARSE_ERROR_EARLY_RETURN(parseResult_);
            while (is.peek() != '\0') {
                Token t = Tokenize(is.peek());
                IterativeParsingState n = Predict(state, t);
                IterativeParsingState d = Transit<parseFlags>(state, t, n, is, handler);

                if (d == IterativeParsingErrorState) {
                    HandleError(state, is);
                    break;
                }

                state = d;

                // Do not further consume streams if a root JSON has been parsed.
                if ((parseFlags & kParseStopWhenDoneFlag) && state == IterativeParsingFinishState) {
                    SetParseError(kParseErrorNone, is.tellg());
                    break;
                }

                skip_whitespace_and_comments<parseFlags>(is);
                MERAK_JSON_PARSE_ERROR_EARLY_RETURN(parseResult_);
            }

            // Handle the end of file.
            if (state != IterativeParsingFinishState)
                HandleError(state, is);

            return parseResult_;
        }

        static const size_t kDefaultStackCapacity = 256;    //!< Default stack capacity in bytes for storing a single decoded string.
        internal::Stack<StackAllocator> stack_;  //!< A stack for storing decoded string temporarily during non-destructive parsing.
        ParseResult parseResult_;
        IterativeParsingState state_;
    }; // class GenericReader

    //! Reader with UTF8 encoding and default allocator.
    typedef GenericReader<UTF8<>, UTF8<> > Reader;

}  // namespace merak::json

#if defined(__clang__) || defined(_MSC_VER)
MERAK_JSON_DIAG_POP
#endif


#ifdef __GNUC__
MERAK_JSON_DIAG_POP
#endif

#endif // MERAK_JSON_READER_H_
