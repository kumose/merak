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

#ifndef MERAK_JSON_FILEREADSTREAM_H_
#define MERAK_JSON_FILEREADSTREAM_H_

#include <merak/json/stream.h>
#include <cstdio>

#ifdef __clang__
MERAK_JSON_DIAG_PUSH
MERAK_JSON_DIAG_OFF(padded)
MERAK_JSON_DIAG_OFF(unreachable-code)
MERAK_JSON_DIAG_OFF(missing-noreturn)
#endif

namespace merak::json {

    //! File byte stream for input using fread().
    /*!
        \note implements Stream concept
    */
    class FileReadStream {
    public:
        typedef char char_type;    //!< Character type (byte).

        //! Constructor.
        /*!
            \param fp File pointer opened for read.
            \param buffer user-supplied buffer.
            \param bufferSize size of buffer in bytes. Must >=4 bytes.
        */
        FileReadStream(std::FILE *fp, char *buffer, size_t bufferSize) : fp_(fp), buffer_(buffer),
                                                                         bufferSize_(bufferSize), bufferLast_(0),
                                                                         current_(buffer_), readCount_(0), count_(0),
                                                                         eof_(false) {
            MERAK_JSON_ASSERT(fp_ != 0);
            MERAK_JSON_ASSERT(bufferSize >= 4);
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
                readCount_ = std::fread(buffer_, 1, bufferSize_, fp_);
                bufferLast_ = buffer_ + readCount_ - 1;
                current_ = buffer_;

                if (readCount_ < bufferSize_) {
                    buffer_[readCount_] = '\0';
                    ++bufferLast_;
                    eof_ = true;
                }
            }
        }

        std::FILE *fp_;
        char_type *buffer_;
        size_t bufferSize_;
        char_type *bufferLast_;
        char_type *current_;
        size_t readCount_;
        size_t count_;  //!< Number of characters read
        bool eof_;
    };

}  // namespace merak::json

#ifdef __clang__
MERAK_JSON_DIAG_POP
#endif

#endif // MERAK_JSON_FILESTREAM_H_
