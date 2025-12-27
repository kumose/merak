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

#ifndef MERAK_JSON_ISTREAMWRAPPER_H_
#define MERAK_JSON_ISTREAMWRAPPER_H_

#include <merak/json/stream.h>
#include <iosfwd>
#include <ios>
#include <vector>
#include <istream>

#ifdef __clang__
MERAK_JSON_DIAG_PUSH
MERAK_JSON_DIAG_OFF(padded)
#elif defined(_MSC_VER)
MERAK_JSON_DIAG_PUSH
MERAK_JSON_DIAG_OFF(4351) // new behavior: elements of array 'array' will be default initialized
#endif

namespace merak::json {

    //! Wrapper of \c std::basic_istream into RapidJSON's Stream concept.
    /*!
        The classes can be wrapped including but not limited to:

        - \c std::istringstream
        - \c std::stringstream
        - \c std::wistringstream
        - \c std::wstringstream
        - \c std::ifstream
        - \c std::fstream
        - \c std::wifstream
        - \c std::wfstream

        \tparam StreamType Class derived from \c std::basic_istream.
    */

    template<typename StreamType>
    class BasicFileInputStream {
    public:
        using char_type = typename StreamType::char_type;

        BasicFileInputStream() = delete;

        BasicFileInputStream(const BasicFileInputStream &) = delete;

        BasicFileInputStream &operator=(const BasicFileInputStream &) = delete;

        //! Constructor.
        /*!
            \param stream stream opened for read.
        */
        BasicFileInputStream(StreamType &stream, size_t bufferSize = 256) : stream_(stream),
                                                                                   peekBuffer_(bufferSize),
                                                                                   buffer_(peekBuffer_.data()),
                                                                                   bufferSize_(bufferSize),
                                                                                   bufferLast_(0), current_(buffer_),
                                                                                   readCount_(0), count_(0),
                                                                                   eof_(false) {
            Read();
        }

        char_type peek() const { return *current_; }

        char_type get() {
            char_type c = *current_;
            Read();
            return c;
        }

        size_t tellg() const { return count_ + static_cast<size_t>(current_ - buffer_); }


    private:
        void Read() {
            if (current_ < bufferLast_)
                ++current_;
            else if (!eof_) {
                count_ += readCount_;
                readCount_ = bufferSize_;
                bufferLast_ = buffer_ + readCount_ - 1;
                current_ = buffer_;

                if (!stream_.read(buffer_, static_cast<std::streamsize>(bufferSize_))) {
                    readCount_ = static_cast<size_t>(stream_.gcount());
                    *(bufferLast_ = buffer_ + readCount_) = '\0';
                    eof_ = true;
                }
            }
        }

        StreamType &stream_;
        std::vector<char_type> peekBuffer_;
        char_type *buffer_;
        size_t bufferSize_;
        char_type *bufferLast_;
        char_type *current_;
        size_t readCount_;
        size_t count_;  //!< Number of characters read
        bool eof_;
    };

    typedef BasicFileInputStream<std::istream> FileInputStream;
    typedef BasicFileInputStream<std::wistream> WFileInputStream;

    template<typename StreamType>
    class BasicStdInputStream {
    public:
        using char_type = typename StreamType::char_type;

        BasicStdInputStream(StreamType &is) : is_(is) {}

        BasicStdInputStream() = delete;
        BasicStdInputStream(const BasicStdInputStream &) = delete;

        BasicStdInputStream &operator=(const BasicStdInputStream &) = delete;

        char_type peek() const {
            int c = is_.peek();
            return c == std::char_traits<char>::eof() ? '\0' : static_cast<char_type>(c);
        }

        char_type get() {
            int c = is_.get();
            return c == std::char_traits<char>::eof() ? '\0' : static_cast<char_type>(c);
        }

        size_t tellg() const { return static_cast<size_t>(is_.tellg()); }

    private:
        StreamType &is_;
    };

    typedef BasicStdInputStream<std::istream> StdInputStream;
    typedef BasicStdInputStream<std::wistream> WStdInputStream;

#if defined(__clang__) || defined(_MSC_VER)
    MERAK_JSON_DIAG_POP
#endif

}  // namespace merak::json

#endif // MERAK_JSON_ISTREAMWRAPPER_H_
