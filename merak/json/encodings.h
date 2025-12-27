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

#ifndef MERAK_JSON_ENCODINGS_H_
#define MERAK_JSON_ENCODINGS_H_

#include <merak/json/json_internal.h>

#if defined(_MSC_VER) && !defined(__clang__)
MERAK_JSON_DIAG_PUSH
MERAK_JSON_DIAG_OFF(4244) // conversion from 'type1' to 'type2', possible loss of data
MERAK_JSON_DIAG_OFF(4702)  // unreachable code
#elif defined(__GNUC__)
MERAK_JSON_DIAG_PUSH
MERAK_JSON_DIAG_OFF(effc++)
MERAK_JSON_DIAG_OFF(overflow)
#endif

namespace merak::json {

///////////////////////////////////////////////////////////////////////////////
// Encoding

/*! \class merak::json::Encoding
    \brief Concept for encoding of Unicode characters.

\code
concept Encoding {
    typename char_type;    //! Type of character. A "character" is actually a code unit in unicode's definition.

    enum { supportUnicode = 1 }; // or 0 if not supporting unicode

    //! \brief Encode a Unicode codepoint to an output stream.
    //! \param os Output stream.
    //! \param codepoint An unicode codepoint, ranging from 0x0 to 0x10FFFF inclusively.
    template<typename OutputStream>
    static void Encode(OutputStream& os, unsigned codepoint);

    //! \brief Decode a Unicode codepoint from an input stream.
    //! \param is Input stream.
    //! \param codepoint Output of the unicode codepoint.
    //! \return true if a valid codepoint can be decoded from the stream.
    template <typename InputStream>
    static bool Decode(InputStream& is, unsigned* codepoint);

    //! \brief Validate one Unicode codepoint from an encoded stream.
    //! \param is Input stream to obtain codepoint.
    //! \param os Output for copying one codepoint.
    //! \return true if it is valid.
    //! \note This function just validating and copying the codepoint without actually decode it.
    template <typename InputStream, typename OutputStream>
    static bool Validate(InputStream& is, OutputStream& os);

    // The following functions are deal with byte streams.

    //! Take a character from input byte stream, skip BOM if exist.
    template <typename InputByteStream>
    static CharType TakeBOM(InputByteStream& is);

    //! Take a character from input byte stream.
    template <typename InputByteStream>
    static char_type Take(InputByteStream& is);

    //! Put BOM to output byte stream.
    template <typename OutputByteStream>
    static void PutBOM(OutputByteStream& os);

    //! Put a character to output byte stream.
    template <typename OutputByteStream>
    static void put(OutputByteStream& os, char_type c);
};
\endcode
*/

///////////////////////////////////////////////////////////////////////////////
// UTF8

//! UTF-8 encoding.
/*! http://en.wikipedia.org/wiki/UTF-8
    http://tools.ietf.org/html/rfc3629
    \tparam CharType Code unit for storing 8-bit UTF-8 data. Default is char.
    \note implements Encoding concept
*/
template<typename CharType = char>
struct UTF8 {
    typedef CharType char_type;

    enum { supportUnicode = 1 };

    template<typename OutputStream>
    static void Encode(OutputStream& os, unsigned codepoint) {
        if (codepoint <= 0x7F) 
            os.put(static_cast<char_type>(codepoint & 0xFF));
        else if (codepoint <= 0x7FF) {
            os.put(static_cast<char_type>(0xC0 | ((codepoint >> 6) & 0xFF)));
            os.put(static_cast<char_type>(0x80 | ((codepoint & 0x3F))));
        }
        else if (codepoint <= 0xFFFF) {
            os.put(static_cast<char_type>(0xE0 | ((codepoint >> 12) & 0xFF)));
            os.put(static_cast<char_type>(0x80 | ((codepoint >> 6) & 0x3F)));
            os.put(static_cast<char_type>(0x80 | (codepoint & 0x3F)));
        }
        else {
            MERAK_JSON_ASSERT(codepoint <= 0x10FFFF);
            os.put(static_cast<char_type>(0xF0 | ((codepoint >> 18) & 0xFF)));
            os.put(static_cast<char_type>(0x80 | ((codepoint >> 12) & 0x3F)));
            os.put(static_cast<char_type>(0x80 | ((codepoint >> 6) & 0x3F)));
            os.put(static_cast<char_type>(0x80 | (codepoint & 0x3F)));
        }
    }

    template<typename OutputStream>
    static void EncodeUnsafe(OutputStream& os, unsigned codepoint) {
        if (codepoint <= 0x7F) 
            PutUnsafe(os, static_cast<char_type>(codepoint & 0xFF));
        else if (codepoint <= 0x7FF) {
            PutUnsafe(os, static_cast<char_type>(0xC0 | ((codepoint >> 6) & 0xFF)));
            PutUnsafe(os, static_cast<char_type>(0x80 | ((codepoint & 0x3F))));
        }
        else if (codepoint <= 0xFFFF) {
            PutUnsafe(os, static_cast<char_type>(0xE0 | ((codepoint >> 12) & 0xFF)));
            PutUnsafe(os, static_cast<char_type>(0x80 | ((codepoint >> 6) & 0x3F)));
            PutUnsafe(os, static_cast<char_type>(0x80 | (codepoint & 0x3F)));
        }
        else {
            MERAK_JSON_ASSERT(codepoint <= 0x10FFFF);
            PutUnsafe(os, static_cast<char_type>(0xF0 | ((codepoint >> 18) & 0xFF)));
            PutUnsafe(os, static_cast<char_type>(0x80 | ((codepoint >> 12) & 0x3F)));
            PutUnsafe(os, static_cast<char_type>(0x80 | ((codepoint >> 6) & 0x3F)));
            PutUnsafe(os, static_cast<char_type>(0x80 | (codepoint & 0x3F)));
        }
    }

    template <typename InputStream>
    static bool Decode(InputStream& is, unsigned* codepoint) {
#define MERAK_JSON_COPY() c = is.get(); *codepoint = (*codepoint << 6) | (static_cast<unsigned char>(c) & 0x3Fu)
#define MERAK_JSON_TRANS(mask) result &= ((GetRange(static_cast<unsigned char>(c)) & mask) != 0)
#define MERAK_JSON_TAIL() MERAK_JSON_COPY(); MERAK_JSON_TRANS(0x70)
        typename InputStream::char_type c = is.get();
        if (!(c & 0x80)) {
            *codepoint = static_cast<unsigned char>(c);
            return true;
        }

        unsigned char type = GetRange(static_cast<unsigned char>(c));
        if (type >= 32) {
            *codepoint = 0;
        } else {
            *codepoint = (0xFFu >> type) & static_cast<unsigned char>(c);
        }
        bool result = true;
        switch (type) {
        case 2: MERAK_JSON_TAIL(); return result;
        case 3: MERAK_JSON_TAIL(); MERAK_JSON_TAIL(); return result;
        case 4: MERAK_JSON_COPY(); MERAK_JSON_TRANS(0x50); MERAK_JSON_TAIL(); return result;
        case 5: MERAK_JSON_COPY(); MERAK_JSON_TRANS(0x10); MERAK_JSON_TAIL(); MERAK_JSON_TAIL(); return result;
        case 6: MERAK_JSON_TAIL(); MERAK_JSON_TAIL(); MERAK_JSON_TAIL(); return result;
        case 10: MERAK_JSON_COPY(); MERAK_JSON_TRANS(0x20); MERAK_JSON_TAIL(); return result;
        case 11: MERAK_JSON_COPY(); MERAK_JSON_TRANS(0x60); MERAK_JSON_TAIL(); MERAK_JSON_TAIL(); return result;
        default: return false;
        }
#undef MERAK_JSON_COPY
#undef MERAK_JSON_TRANS
#undef MERAK_JSON_TAIL
    }

    template <typename InputStream, typename OutputStream>
    static bool Validate(InputStream& is, OutputStream& os) {
#define MERAK_JSON_COPY() if (c != '\0') os.put(c = is.get())
#define MERAK_JSON_TRANS(mask) result &= ((GetRange(static_cast<unsigned char>(c)) & mask) != 0)
#define MERAK_JSON_TAIL() MERAK_JSON_COPY(); MERAK_JSON_TRANS(0x70)
        char_type c = static_cast<char_type>(-1);
        MERAK_JSON_COPY();
        if (!(c & 0x80))
            return true;

        bool result = true;
        switch (GetRange(static_cast<unsigned char>(c))) {
        case 2: MERAK_JSON_TAIL(); return result;
        case 3: MERAK_JSON_TAIL(); MERAK_JSON_TAIL(); return result;
        case 4: MERAK_JSON_COPY(); MERAK_JSON_TRANS(0x50); MERAK_JSON_TAIL(); return result;
        case 5: MERAK_JSON_COPY(); MERAK_JSON_TRANS(0x10); MERAK_JSON_TAIL(); MERAK_JSON_TAIL(); return result;
        case 6: MERAK_JSON_TAIL(); MERAK_JSON_TAIL(); MERAK_JSON_TAIL(); return result;
        case 10: MERAK_JSON_COPY(); MERAK_JSON_TRANS(0x20); MERAK_JSON_TAIL(); return result;
        case 11: MERAK_JSON_COPY(); MERAK_JSON_TRANS(0x60); MERAK_JSON_TAIL(); MERAK_JSON_TAIL(); return result;
        default: return false;
        }
#undef MERAK_JSON_COPY
#undef MERAK_JSON_TRANS
#undef MERAK_JSON_TAIL
    }

    static unsigned char GetRange(unsigned char c) {
        // Referring to DFA of http://bjoern.hoehrmann.de/utf-8/decoder/dfa/
        // With new mapping 1 -> 0x10, 7 -> 0x20, 9 -> 0x40, such that AND operation can test multiple types.
        static const unsigned char type[] = {
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
            0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,
            0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,
            0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
            0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
            8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
            10,3,3,3,3,3,3,3,3,3,3,3,3,4,3,3, 11,6,6,6,5,8,8,8,8,8,8,8,8,8,8,8,
        };
        return type[c];
    }

    template <typename InputByteStream>
    static CharType TakeBOM(InputByteStream& is) {
        MERAK_JSON_STATIC_ASSERT(sizeof(typename InputByteStream::char_type) == 1);
        typename InputByteStream::char_type c = Take(is);
        if (static_cast<unsigned char>(c) != 0xEFu) return c;
        c = is.get();
        if (static_cast<unsigned char>(c) != 0xBBu) return c;
        c = is.get();
        if (static_cast<unsigned char>(c) != 0xBFu) return c;
        c = is.get();
        return c;
    }

    template <typename InputByteStream>
    static char_type Take(InputByteStream& is) {
        MERAK_JSON_STATIC_ASSERT(sizeof(typename InputByteStream::char_type) == 1);
        return static_cast<char_type>(is.get());
    }

    template <typename OutputByteStream>
    static void PutBOM(OutputByteStream& os) {
        MERAK_JSON_STATIC_ASSERT(sizeof(typename OutputByteStream::char_type) == 1);
        os.put(static_cast<typename OutputByteStream::char_type>(0xEFu));
        os.put(static_cast<typename OutputByteStream::char_type>(0xBBu));
        os.put(static_cast<typename OutputByteStream::char_type>(0xBFu));
    }

    template <typename OutputByteStream>
    static void put(OutputByteStream& os, char_type c) {
        MERAK_JSON_STATIC_ASSERT(sizeof(typename OutputByteStream::char_type) == 1);
        os.put(static_cast<typename OutputByteStream::char_type>(c));
    }
};

///////////////////////////////////////////////////////////////////////////////
// UTF16

//! UTF-16 encoding.
/*! http://en.wikipedia.org/wiki/UTF-16
    http://tools.ietf.org/html/rfc2781
    \tparam CharType Type for storing 16-bit UTF-16 data. Default is wchar_t. C++11 may use char16_t instead.
    \note implements Encoding concept

    \note For in-memory access, no need to concern endianness. The code units and code points are represented by CPU's endianness.
    For streaming, use UTF16LE and UTF16BE, which handle endianness.
*/
template<typename CharType = wchar_t>
struct UTF16 {
    typedef CharType char_type;
    MERAK_JSON_STATIC_ASSERT(sizeof(char_type) >= 2);

    enum { supportUnicode = 1 };

    template<typename OutputStream>
    static void Encode(OutputStream& os, unsigned codepoint) {
        MERAK_JSON_STATIC_ASSERT(sizeof(typename OutputStream::char_type) >= 2);
        if (codepoint <= 0xFFFF) {
            MERAK_JSON_ASSERT(codepoint < 0xD800 || codepoint > 0xDFFF); // Code point itself cannot be surrogate pair
            os.put(static_cast<typename OutputStream::char_type>(codepoint));
        }
        else {
            MERAK_JSON_ASSERT(codepoint <= 0x10FFFF);
            unsigned v = codepoint - 0x10000;
            os.put(static_cast<typename OutputStream::char_type>((v >> 10) | 0xD800));
            os.put(static_cast<typename OutputStream::char_type>((v & 0x3FF) | 0xDC00));
        }
    }


    template<typename OutputStream>
    static void EncodeUnsafe(OutputStream& os, unsigned codepoint) {
        MERAK_JSON_STATIC_ASSERT(sizeof(typename OutputStream::char_type) >= 2);
        if (codepoint <= 0xFFFF) {
            MERAK_JSON_ASSERT(codepoint < 0xD800 || codepoint > 0xDFFF); // Code point itself cannot be surrogate pair
            PutUnsafe(os, static_cast<typename OutputStream::char_type>(codepoint));
        }
        else {
            MERAK_JSON_ASSERT(codepoint <= 0x10FFFF);
            unsigned v = codepoint - 0x10000;
            PutUnsafe(os, static_cast<typename OutputStream::char_type>((v >> 10) | 0xD800));
            PutUnsafe(os, static_cast<typename OutputStream::char_type>((v & 0x3FF) | 0xDC00));
        }
    }

    template <typename InputStream>
    static bool Decode(InputStream& is, unsigned* codepoint) {
        MERAK_JSON_STATIC_ASSERT(sizeof(typename InputStream::char_type) >= 2);
        typename InputStream::char_type c = is.get();
        if (c < 0xD800 || c > 0xDFFF) {
            *codepoint = static_cast<unsigned>(c);
            return true;
        }
        else if (c <= 0xDBFF) {
            *codepoint = (static_cast<unsigned>(c) & 0x3FF) << 10;
            c = is.get();
            *codepoint |= (static_cast<unsigned>(c) & 0x3FF);
            *codepoint += 0x10000;
            return c >= 0xDC00 && c <= 0xDFFF;
        }
        return false;
    }

    template <typename InputStream, typename OutputStream>
    static bool Validate(InputStream& is, OutputStream& os) {
        MERAK_JSON_STATIC_ASSERT(sizeof(typename InputStream::char_type) >= 2);
        MERAK_JSON_STATIC_ASSERT(sizeof(typename OutputStream::char_type) >= 2);
        typename InputStream::char_type c;
        os.put(static_cast<typename OutputStream::char_type>(c = is.get()));
        if (c < 0xD800 || c > 0xDFFF)
            return true;
        else if (c <= 0xDBFF) {
            os.put(c = is.get());
            return c >= 0xDC00 && c <= 0xDFFF;
        }
        return false;
    }
};

//! UTF-16 little endian encoding.
template<typename CharType = wchar_t>
struct UTF16LE : UTF16<CharType> {
    template <typename InputByteStream>
    static CharType TakeBOM(InputByteStream& is) {
        MERAK_JSON_STATIC_ASSERT(sizeof(typename InputByteStream::char_type) == 1);
        CharType c = Take(is);
        return static_cast<uint16_t>(c) == 0xFEFFu ? Take(is) : c;
    }

    template <typename InputByteStream>
    static CharType Take(InputByteStream& is) {
        MERAK_JSON_STATIC_ASSERT(sizeof(typename InputByteStream::char_type) == 1);
        unsigned c = static_cast<uint8_t>(is.get());
        c |= static_cast<unsigned>(static_cast<uint8_t>(is.get())) << 8;
        return static_cast<CharType>(c);
    }

    template <typename OutputByteStream>
    static void PutBOM(OutputByteStream& os) {
        MERAK_JSON_STATIC_ASSERT(sizeof(typename OutputByteStream::char_type) == 1);
        os.put(static_cast<typename OutputByteStream::char_type>(0xFFu));
        os.put(static_cast<typename OutputByteStream::char_type>(0xFEu));
    }

    template <typename OutputByteStream>
    static void put(OutputByteStream& os, CharType c) {
        MERAK_JSON_STATIC_ASSERT(sizeof(typename OutputByteStream::char_type) == 1);
        os.put(static_cast<typename OutputByteStream::char_type>(static_cast<unsigned>(c) & 0xFFu));
        os.put(static_cast<typename OutputByteStream::char_type>((static_cast<unsigned>(c) >> 8) & 0xFFu));
    }
};

//! UTF-16 big endian encoding.
template<typename CharType = wchar_t>
struct UTF16BE : UTF16<CharType> {
    template <typename InputByteStream>
    static CharType TakeBOM(InputByteStream& is) {
        MERAK_JSON_STATIC_ASSERT(sizeof(typename InputByteStream::char_type) == 1);
        CharType c = Take(is);
        return static_cast<uint16_t>(c) == 0xFEFFu ? Take(is) : c;
    }

    template <typename InputByteStream>
    static CharType Take(InputByteStream& is) {
        MERAK_JSON_STATIC_ASSERT(sizeof(typename InputByteStream::char_type) == 1);
        unsigned c = static_cast<unsigned>(static_cast<uint8_t>(is.get())) << 8;
        c |= static_cast<unsigned>(static_cast<uint8_t>(is.get()));
        return static_cast<CharType>(c);
    }

    template <typename OutputByteStream>
    static void PutBOM(OutputByteStream& os) {
        MERAK_JSON_STATIC_ASSERT(sizeof(typename OutputByteStream::char_type) == 1);
        os.put(static_cast<typename OutputByteStream::char_type>(0xFEu));
        os.put(static_cast<typename OutputByteStream::char_type>(0xFFu));
    }

    template <typename OutputByteStream>
    static void put(OutputByteStream& os, CharType c) {
        MERAK_JSON_STATIC_ASSERT(sizeof(typename OutputByteStream::char_type) == 1);
        os.put(static_cast<typename OutputByteStream::char_type>((static_cast<unsigned>(c) >> 8) & 0xFFu));
        os.put(static_cast<typename OutputByteStream::char_type>(static_cast<unsigned>(c) & 0xFFu));
    }
};

///////////////////////////////////////////////////////////////////////////////
// UTF32

//! UTF-32 encoding. 
/*! http://en.wikipedia.org/wiki/UTF-32
    \tparam CharType Type for storing 32-bit UTF-32 data. Default is unsigned. C++11 may use char32_t instead.
    \note implements Encoding concept

    \note For in-memory access, no need to concern endianness. The code units and code points are represented by CPU's endianness.
    For streaming, use UTF32LE and UTF32BE, which handle endianness.
*/
template<typename CharType = unsigned>
struct UTF32 {
    typedef CharType char_type;
    MERAK_JSON_STATIC_ASSERT(sizeof(char_type) >= 4);

    enum { supportUnicode = 1 };

    template<typename OutputStream>
    static void Encode(OutputStream& os, unsigned codepoint) {
        MERAK_JSON_STATIC_ASSERT(sizeof(typename OutputStream::char_type) >= 4);
        MERAK_JSON_ASSERT(codepoint <= 0x10FFFF);
        os.put(codepoint);
    }

    template<typename OutputStream>
    static void EncodeUnsafe(OutputStream& os, unsigned codepoint) {
        MERAK_JSON_STATIC_ASSERT(sizeof(typename OutputStream::char_type) >= 4);
        MERAK_JSON_ASSERT(codepoint <= 0x10FFFF);
        PutUnsafe(os, codepoint);
    }

    template <typename InputStream>
    static bool Decode(InputStream& is, unsigned* codepoint) {
        MERAK_JSON_STATIC_ASSERT(sizeof(typename InputStream::char_type) >= 4);
        char_type c = is.get();
        *codepoint = c;
        return c <= 0x10FFFF;
    }

    template <typename InputStream, typename OutputStream>
    static bool Validate(InputStream& is, OutputStream& os) {
        MERAK_JSON_STATIC_ASSERT(sizeof(typename InputStream::char_type) >= 4);
        char_type c;
        os.put(c = is.get());
        return c <= 0x10FFFF;
    }
};

//! UTF-32 little endian enocoding.
template<typename CharType = unsigned>
struct UTF32LE : UTF32<CharType> {
    template <typename InputByteStream>
    static CharType TakeBOM(InputByteStream& is) {
        MERAK_JSON_STATIC_ASSERT(sizeof(typename InputByteStream::char_type) == 1);
        CharType c = Take(is);
        return static_cast<uint32_t>(c) == 0x0000FEFFu ? Take(is) : c;
    }

    template <typename InputByteStream>
    static CharType Take(InputByteStream& is) {
        MERAK_JSON_STATIC_ASSERT(sizeof(typename InputByteStream::char_type) == 1);
        unsigned c = static_cast<uint8_t>(is.get());
        c |= static_cast<unsigned>(static_cast<uint8_t>(is.get())) << 8;
        c |= static_cast<unsigned>(static_cast<uint8_t>(is.get())) << 16;
        c |= static_cast<unsigned>(static_cast<uint8_t>(is.get())) << 24;
        return static_cast<CharType>(c);
    }

    template <typename OutputByteStream>
    static void PutBOM(OutputByteStream& os) {
        MERAK_JSON_STATIC_ASSERT(sizeof(typename OutputByteStream::char_type) == 1);
        os.put(static_cast<typename OutputByteStream::char_type>(0xFFu));
        os.put(static_cast<typename OutputByteStream::char_type>(0xFEu));
        os.put(static_cast<typename OutputByteStream::char_type>(0x00u));
        os.put(static_cast<typename OutputByteStream::char_type>(0x00u));
    }

    template <typename OutputByteStream>
    static void put(OutputByteStream& os, CharType c) {
        MERAK_JSON_STATIC_ASSERT(sizeof(typename OutputByteStream::char_type) == 1);
        os.put(static_cast<typename OutputByteStream::char_type>(c & 0xFFu));
        os.put(static_cast<typename OutputByteStream::char_type>((c >> 8) & 0xFFu));
        os.put(static_cast<typename OutputByteStream::char_type>((c >> 16) & 0xFFu));
        os.put(static_cast<typename OutputByteStream::char_type>((c >> 24) & 0xFFu));
    }
};

//! UTF-32 big endian encoding.
template<typename CharType = unsigned>
struct UTF32BE : UTF32<CharType> {
    template <typename InputByteStream>
    static CharType TakeBOM(InputByteStream& is) {
        MERAK_JSON_STATIC_ASSERT(sizeof(typename InputByteStream::char_type) == 1);
        CharType c = Take(is);
        return static_cast<uint32_t>(c) == 0x0000FEFFu ? Take(is) : c; 
    }

    template <typename InputByteStream>
    static CharType Take(InputByteStream& is) {
        MERAK_JSON_STATIC_ASSERT(sizeof(typename InputByteStream::char_type) == 1);
        unsigned c = static_cast<unsigned>(static_cast<uint8_t>(is.get())) << 24;
        c |= static_cast<unsigned>(static_cast<uint8_t>(is.get())) << 16;
        c |= static_cast<unsigned>(static_cast<uint8_t>(is.get())) << 8;
        c |= static_cast<unsigned>(static_cast<uint8_t>(is.get()));
        return static_cast<CharType>(c);
    }

    template <typename OutputByteStream>
    static void PutBOM(OutputByteStream& os) {
        MERAK_JSON_STATIC_ASSERT(sizeof(typename OutputByteStream::char_type) == 1);
        os.put(static_cast<typename OutputByteStream::char_type>(0x00u));
        os.put(static_cast<typename OutputByteStream::char_type>(0x00u));
        os.put(static_cast<typename OutputByteStream::char_type>(0xFEu));
        os.put(static_cast<typename OutputByteStream::char_type>(0xFFu));
    }

    template <typename OutputByteStream>
    static void put(OutputByteStream& os, CharType c) {
        MERAK_JSON_STATIC_ASSERT(sizeof(typename OutputByteStream::char_type) == 1);
        os.put(static_cast<typename OutputByteStream::char_type>((c >> 24) & 0xFFu));
        os.put(static_cast<typename OutputByteStream::char_type>((c >> 16) & 0xFFu));
        os.put(static_cast<typename OutputByteStream::char_type>((c >> 8) & 0xFFu));
        os.put(static_cast<typename OutputByteStream::char_type>(c & 0xFFu));
    }
};

///////////////////////////////////////////////////////////////////////////////
// ASCII

//! ASCII encoding.
/*! http://en.wikipedia.org/wiki/ASCII
    \tparam CharType Code unit for storing 7-bit ASCII data. Default is char.
    \note implements Encoding concept
*/
template<typename CharType = char>
struct ASCII {
    typedef CharType char_type;

    enum { supportUnicode = 0 };

    template<typename OutputStream>
    static void Encode(OutputStream& os, unsigned codepoint) {
        MERAK_JSON_ASSERT(codepoint <= 0x7F);
        os.put(static_cast<char_type>(codepoint & 0xFF));
    }

    template<typename OutputStream>
    static void EncodeUnsafe(OutputStream& os, unsigned codepoint) {
        MERAK_JSON_ASSERT(codepoint <= 0x7F);
        PutUnsafe(os, static_cast<char_type>(codepoint & 0xFF));
    }

    template <typename InputStream>
    static bool Decode(InputStream& is, unsigned* codepoint) {
        uint8_t c = static_cast<uint8_t>(is.get());
        *codepoint = c;
        return c <= 0X7F;
    }

    template <typename InputStream, typename OutputStream>
    static bool Validate(InputStream& is, OutputStream& os) {
        uint8_t c = static_cast<uint8_t>(is.get());
        os.put(static_cast<typename OutputStream::char_type>(c));
        return c <= 0x7F;
    }

    template <typename InputByteStream>
    static CharType TakeBOM(InputByteStream& is) {
        MERAK_JSON_STATIC_ASSERT(sizeof(typename InputByteStream::char_type) == 1);
        uint8_t c = static_cast<uint8_t>(Take(is));
        return static_cast<char_type>(c);
    }

    template <typename InputByteStream>
    static char_type Take(InputByteStream& is) {
        MERAK_JSON_STATIC_ASSERT(sizeof(typename InputByteStream::char_type) == 1);
        return static_cast<char_type>(is.get());
    }

    template <typename OutputByteStream>
    static void PutBOM(OutputByteStream& os) {
        MERAK_JSON_STATIC_ASSERT(sizeof(typename OutputByteStream::char_type) == 1);
        (void)os;
    }

    template <typename OutputByteStream>
    static void put(OutputByteStream& os, char_type c) {
        MERAK_JSON_STATIC_ASSERT(sizeof(typename OutputByteStream::char_type) == 1);
        os.put(static_cast<typename OutputByteStream::char_type>(c));
    }
};

///////////////////////////////////////////////////////////////////////////////
// AutoUTF

//! Runtime-specified UTF encoding type of a stream.
enum UTFType {
    kUTF8 = 0,      //!< UTF-8.
    kUTF16LE = 1,   //!< UTF-16 little endian.
    kUTF16BE = 2,   //!< UTF-16 big endian.
    kUTF32LE = 3,   //!< UTF-32 little endian.
    kUTF32BE = 4    //!< UTF-32 big endian.
};

//! Dynamically select encoding according to stream's runtime-specified UTF encoding type.
/*! \note This class can be used with AutoUTFInputtStream and AutoUTFOutputStream, which provides get_type().
*/
template<typename CharType>
struct AutoUTF {
    typedef CharType char_type;

    enum { supportUnicode = 1 };

#define MERAK_JSON_ENCODINGS_FUNC(x) UTF8<char_type>::x, UTF16LE<char_type>::x, UTF16BE<char_type>::x, UTF32LE<char_type>::x, UTF32BE<char_type>::x

    template<typename OutputStream>
    static MERAK_JSON_FORCEINLINE void Encode(OutputStream& os, unsigned codepoint) {
        typedef void (*EncodeFunc)(OutputStream&, unsigned);
        static const EncodeFunc f[] = { MERAK_JSON_ENCODINGS_FUNC(Encode) };
        (*f[os.get_type()])(os, codepoint);
    }

    template<typename OutputStream>
    static MERAK_JSON_FORCEINLINE void EncodeUnsafe(OutputStream& os, unsigned codepoint) {
        typedef void (*EncodeFunc)(OutputStream&, unsigned);
        static const EncodeFunc f[] = { MERAK_JSON_ENCODINGS_FUNC(EncodeUnsafe) };
        (*f[os.get_type()])(os, codepoint);
    }

    template <typename InputStream>
    static MERAK_JSON_FORCEINLINE bool Decode(InputStream& is, unsigned* codepoint) {
        typedef bool (*DecodeFunc)(InputStream&, unsigned*);
        static const DecodeFunc f[] = { MERAK_JSON_ENCODINGS_FUNC(Decode) };
        return (*f[is.get_type()])(is, codepoint);
    }

    template <typename InputStream, typename OutputStream>
    static MERAK_JSON_FORCEINLINE bool Validate(InputStream& is, OutputStream& os) {
        typedef bool (*ValidateFunc)(InputStream&, OutputStream&);
        static const ValidateFunc f[] = { MERAK_JSON_ENCODINGS_FUNC(Validate) };
        return (*f[is.get_type()])(is, os);
    }

#undef MERAK_JSON_ENCODINGS_FUNC
};

///////////////////////////////////////////////////////////////////////////////
// Transcoder

//! Encoding conversion.
template<typename SourceEncoding, typename TargetEncoding>
struct Transcoder {
    //! Take one Unicode codepoint from source encoding, convert it to target encoding and put it to the output stream.
    template<typename InputStream, typename OutputStream>
    static MERAK_JSON_FORCEINLINE bool Transcode(InputStream& is, OutputStream& os) {
        unsigned codepoint;
        if (!SourceEncoding::Decode(is, &codepoint))
            return false;
        TargetEncoding::Encode(os, codepoint);
        return true;
    }

    template<typename InputStream, typename OutputStream>
    static MERAK_JSON_FORCEINLINE bool TranscodeUnsafe(InputStream& is, OutputStream& os) {
        unsigned codepoint;
        if (!SourceEncoding::Decode(is, &codepoint))
            return false;
        TargetEncoding::EncodeUnsafe(os, codepoint);
        return true;
    }

    //! Validate one Unicode codepoint from an encoded stream.
    template<typename InputStream, typename OutputStream>
    static MERAK_JSON_FORCEINLINE bool Validate(InputStream& is, OutputStream& os) {
        return Transcode(is, os);   // Since source/target encoding is different, must transcode.
    }
};

// Forward declaration.
template<typename Stream>
inline void PutUnsafe(Stream& stream, typename Stream::char_type c);

//! Specialization of Transcoder with same source and target encoding.
template<typename Encoding>
struct Transcoder<Encoding, Encoding> {
    template<typename InputStream, typename OutputStream>
    static MERAK_JSON_FORCEINLINE bool Transcode(InputStream& is, OutputStream& os) {
        os.put(is.get());  // Just copy one code unit. This semantic is different from primary template class.
        return true;
    }
    
    template<typename InputStream, typename OutputStream>
    static MERAK_JSON_FORCEINLINE bool TranscodeUnsafe(InputStream& is, OutputStream& os) {
        PutUnsafe(os, is.get());  // Just copy one code unit. This semantic is different from primary template class.
        return true;
    }
    
    template<typename InputStream, typename OutputStream>
    static MERAK_JSON_FORCEINLINE bool Validate(InputStream& is, OutputStream& os) {
        return Encoding::Validate(is, os);  // source/target encoding are the same
    }
};

}  // namespace merak::json

#if defined(__GNUC__) || (defined(_MSC_VER) && !defined(__clang__))
MERAK_JSON_DIAG_POP
#endif

#endif // MERAK_JSON_ENCODINGS_H_
