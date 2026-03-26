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
