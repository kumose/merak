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

#ifndef MERAK_JSON_ENCODEDSTREAM_H_
#define MERAK_JSON_ENCODEDSTREAM_H_

#include <merak/json/stream.h>
#include <merak/json/memorystream.h>

#ifdef __GNUC__
MERAK_JSON_DIAG_PUSH
MERAK_JSON_DIAG_OFF(effc++)
#endif

#ifdef __clang__
MERAK_JSON_DIAG_PUSH
MERAK_JSON_DIAG_OFF(padded)
#endif

namespace merak::json {

    //! Input byte stream wrapper with a statically bound encoding.
    /*!
        \tparam Encoding The interpretation of encoding of the stream. Either UTF8, UTF16LE, UTF16BE, UTF32LE, UTF32BE.
        \tparam InputByteStream Type of input byte stream. For example, FileReadStream.
    */
    template<typename Encoding, typename InputByteStream>
    class EncodedInputStream {
        MERAK_JSON_STATIC_ASSERT(sizeof(typename InputByteStream::char_type) == 1);
    public:
        typedef typename Encoding::char_type char_type;

        EncodedInputStream(InputByteStream &is) : is_(is) {
            current_ = Encoding::TakeBOM(is_);
        }

        char_type peek() const { return current_; }

        char_type get() {
            char_type c = current_;
            current_ = Encoding::Take(is_);
            return c;
        }

        size_t tellg() const { return is_.tellg(); }


    private:
        EncodedInputStream(const EncodedInputStream &);

        EncodedInputStream &operator=(const EncodedInputStream &);

        InputByteStream &is_;
        char_type current_;
    };

    //! Specialized for UTF8 MemoryStream.
    template<>
    class EncodedInputStream<UTF8<>, MemoryStream> {
    public:
        typedef UTF8<>::char_type char_type;

        EncodedInputStream(MemoryStream &is) : is_(is) {
            if (static_cast<unsigned char>(is_.peek()) == 0xEFu) is_.get();
            if (static_cast<unsigned char>(is_.peek()) == 0xBBu) is_.get();
            if (static_cast<unsigned char>(is_.peek()) == 0xBFu) is_.get();
        }

        char_type peek() const { return is_.peek(); }

        char_type get() { return is_.get(); }

        size_t tellg() const { return is_.tellg(); }

        MemoryStream &is_;

    private:
        EncodedInputStream(const EncodedInputStream &);

        EncodedInputStream &operator=(const EncodedInputStream &);
    };

    //! Output byte stream wrapper with statically bound encoding.
    /*!
        \tparam Encoding The interpretation of encoding of the stream. Either UTF8, UTF16LE, UTF16BE, UTF32LE, UTF32BE.
        \tparam OutputByteStream Type of input byte stream. For example, FileWriteStream.
    */
    template<typename Encoding, typename OutputByteStream>
    class EncodedOutputStream {
        MERAK_JSON_STATIC_ASSERT(sizeof(typename OutputByteStream::char_type) == 1);
    public:
        typedef typename Encoding::char_type char_type;

        EncodedOutputStream(OutputByteStream &os, bool putBOM = true) : os_(os) {
            if (putBOM)
                Encoding::PutBOM(os_);
        }

        void put(char_type c) { Encoding::put(os_, c); }

        //void write(const char_type *c, size_t length) { Encoding::write(os_, c, length); }

        EncodedOutputStream &flush() {
            os_.flush();
            return *this;
        }

        // Not implemented
        char_type peek() const {
            MERAK_JSON_ASSERT(false);
            return 0;
        }

        char_type get() {
            MERAK_JSON_ASSERT(false);
            return 0;
        }

        size_t tellg() const {
            MERAK_JSON_ASSERT(false);
            return 0;
        }

    private:
        EncodedOutputStream(const EncodedOutputStream &);

        EncodedOutputStream &operator=(const EncodedOutputStream &);

        OutputByteStream &os_;
    };

}  // namespace merak::json

#ifdef __clang__
MERAK_JSON_DIAG_POP
#endif

#ifdef __GNUC__
MERAK_JSON_DIAG_POP
#endif

#endif // MERAK_JSON_FILESTREAM_H_
