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


#include <merak/json/json_internal.h>

#ifndef MERAK_JSON_STREAM_H_
#define MERAK_JSON_STREAM_H_

#include <merak/json/encodings.h>

namespace merak::json {

    ///////////////////////////////////////////////////////////////////////////////
    //  Stream

    /*! \class merak::json::Stream
        \brief Concept for reading and writing characters.

        For read-only stream, no need to implement PutBegin(), put(), flush() and PutEnd().

        For write-only stream, only need to implement put() and flush().

    \code
    concept Stream {
        typename char_type;    //!< Character type of the stream.

        //! Read the current character from stream without moving the read cursor.
        char_type peek() const;

        //! Read the current character from stream and moving the read cursor to next character.
        char_type get();

        //! Get the current read cursor.
        //! \return Number of characters read from start.
        size_t tellg();

        //! begin writing operation at the current read pointer.
        //! \return The begin writer pointer.
        char_type* PutBegin();

        //! Write a character.
        void put(char_type c);

        //! Flush the buffer.
        Stream &flush();

        //! end the writing operation.
        //! \param begin The begin write pointer returned by PutBegin().
        //! \return Number of characters written.
        size_t PutEnd(char_type* begin);
    }
    \endcode
    */

    //! Provides additional information for stream.
    /*!
        By using traits pattern, this type provides a default configuration for stream.
        For custom stream, this type can be specialized for other configuration.
        See TEST(Reader, CustomStringStream) in readertest.cpp for example.
    */
    template<typename Stream>
    struct StreamTraits {
        //! Whether to make local copy of stream for optimization during parsing.
        /*!
            By default, for safety, streams do not use local copy optimization.
            Stream that can be copied fast should specialize this, like StreamTraits<StringStream>.
        */
        enum {
            copyOptimization = 0
        };
    };

    //! reserve n characters for writing to a stream.
    template<typename Stream>
    inline void PutReserve(Stream &stream, size_t count) {
        (void) stream;
        (void) count;
    }

    //! Write character to a stream, presuming buffer is reserved.
    template<typename Stream>
    inline void PutUnsafe(Stream &stream, typename Stream::char_type c) {
        stream.put(c);
    }

    //! Put N copies of a character to a stream.
    template<typename Stream, typename char_type>
    inline void PutN(Stream &stream, char_type c, size_t n) {
        PutReserve(stream, n);
        for (size_t i = 0; i < n; i++)
            PutUnsafe(stream, c);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // GenericStreamWrapper

    //! A Stream Wrapper
    /*! \tThis string stream is a wrapper for any stream by just forwarding any
        \treceived message to the origin stream.
        \note implements Stream concept
    */

#if defined(_MSC_VER) && _MSC_VER <= 1800
    MERAK_JSON_DIAG_PUSH
    MERAK_JSON_DIAG_OFF(4702)  // unreachable code
    MERAK_JSON_DIAG_OFF(4512)  // assignment operator could not be generated
#endif

    template<typename InputStream, typename Encoding = UTF8<> >
    class GenericStreamWrapper {
    public:
        typedef typename Encoding::char_type char_type;

        GenericStreamWrapper(InputStream &is) : is_(is) {}

        char_type peek() const { return is_.peek(); }

        char_type get() { return is_.get(); }

        size_t tellg() { return is_.tellg(); }


        void put(char_type ch) { is_.put(ch); }

        GenericStreamWrapper flush() { is_.flush();
            return *this;
        }

        UTFType get_type() const { return is_.get_type(); }

        bool HasBOM() const { return is_.HasBOM(); }

    protected:
        InputStream &is_;
    };

#if defined(_MSC_VER) && _MSC_VER <= 1800
    MERAK_JSON_DIAG_POP
#endif

    ///////////////////////////////////////////////////////////////////////////////
    // StringStream

    //! Read-only string stream.
    /*! \note implements Stream concept
    */
    template<typename Encoding>
    struct GenericStringStream {
        typedef typename Encoding::char_type char_type;

        GenericStringStream(const char_type *src) : src_(src), head_(src) {}

        char_type peek() const { return *src_; }

        char_type get() { return *src_++; }

        size_t tellg() const { return static_cast<size_t>(src_ - head_); }


        const char_type *src_;     //!< Current read position.
        const char_type *head_;    //!< Original head of the string.
    };

    template<typename Encoding>
    struct StreamTraits<GenericStringStream<Encoding> > {
        enum {
            copyOptimization = 1
        };
    };

    //! String stream with UTF8 encoding.
    typedef GenericStringStream<UTF8<> > StringStream;

    ///////////////////////////////////////////////////////////////////////////////
    // InsituStringStream

    //! A read-write string stream.
    /*! This string stream is particularly designed for in-situ parsing.
        \note implements Stream concept
    */
    template<typename Encoding>
    struct GenericInsituStringStream {
        typedef typename Encoding::char_type char_type;

        GenericInsituStringStream(char_type *src) : src_(src), dst_(0), head_(src) {}

        // Read
        char_type peek() { return *src_; }

        char_type get() { return *src_++; }

        size_t tellg() { return static_cast<size_t>(src_ - head_); }

        // Write
        void put(char_type c) {
            MERAK_JSON_ASSERT(dst_ != 0);
            *dst_++ = c;
        }

        char_type *PutBegin() { return dst_ = src_; }

        size_t PutEnd(char_type *begin) { return static_cast<size_t>(dst_ - begin); }

        GenericInsituStringStream &flush() {
            return *this;
        }

        char_type *Push(size_t count) {
            char_type *begin = dst_;
            dst_ += count;
            return begin;
        }

        void Pop(size_t count) { dst_ -= count; }

        char_type *src_;
        char_type *dst_;
        char_type *head_;
    };

    template<typename Encoding>
    struct StreamTraits<GenericInsituStringStream<Encoding> > {
        enum {
            copyOptimization = 1
        };
    };

    //! Insitu string stream with UTF8 encoding.
    typedef GenericInsituStringStream<UTF8<> > InsituStringStream;

}  // namespace merak::json

#endif // MERAK_JSON_STREAM_H_
