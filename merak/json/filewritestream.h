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

#ifndef MERAK_JSON_FILEWRITESTREAM_H_
#define MERAK_JSON_FILEWRITESTREAM_H_

#include <merak/json/stream.h>
#include <cstdio>

#ifdef __clang__
MERAK_JSON_DIAG_PUSH
MERAK_JSON_DIAG_OFF(unreachable-code)
#endif

namespace merak::json {

//! Wrapper of C file stream for output using fwrite().
/*!
    \note implements Stream concept
*/
class FileWriteStream {
public:
    typedef char char_type;    //!< Character type. Only support char.

    FileWriteStream(std::FILE* fp, char* buffer, size_t bufferSize) : fp_(fp), buffer_(buffer), bufferEnd_(buffer + bufferSize), current_(buffer_) { 
        MERAK_JSON_ASSERT(fp_ != 0);
    }

    void put(char c) {
        if (current_ >= bufferEnd_)
            flush();

        *current_++ = c;
    }


    void write(const char_type* c, size_t length) {
        size_t avail = static_cast<size_t>(bufferEnd_ - current_);
        while (length > avail) {
            std::memcpy(current_, c, avail);
            current_ += avail;
            flush();
            length -= avail;
            avail = static_cast<size_t>(bufferEnd_ - current_);
        }

        if (length > 0) {
            std::memcpy(current_, c, length);
            current_ += length;
        }
    }


    void PutN(char c, size_t n) {
        size_t avail = static_cast<size_t>(bufferEnd_ - current_);
        while (n > avail) {
            std::memset(current_, c, avail);
            current_ += avail;
            flush();
            n -= avail;
            avail = static_cast<size_t>(bufferEnd_ - current_);
        }

        if (n > 0) {
            std::memset(current_, c, n);
            current_ += n;
        }
    }

    FileWriteStream &flush() {
        if (current_ != buffer_) {
            size_t result = std::fwrite(buffer_, 1, static_cast<size_t>(current_ - buffer_), fp_);
            if (result < static_cast<size_t>(current_ - buffer_)) {
                // failure deliberately ignored at this time
                // added to avoid warn_unused_result build errors
            }
            current_ = buffer_;
        }
        return *this;
    }

    // Not implemented
    char peek() const { MERAK_JSON_ASSERT(false); return 0; }
    char get() { MERAK_JSON_ASSERT(false); return 0; }
    size_t tellg() const { MERAK_JSON_ASSERT(false); return 0; }

private:
    // Prohibit copy constructor & assignment operator.
    FileWriteStream(const FileWriteStream&);
    FileWriteStream& operator=(const FileWriteStream&);

    std::FILE* fp_;
    char *buffer_;
    char *bufferEnd_;
    char *current_;
};

//! Implement specialized version of PutN() with memset() for better performance.
template<>
inline void PutN(FileWriteStream& stream, char c, size_t n) {
    stream.PutN(c, n);
}

}  // namespace merak::json

#ifdef __clang__
MERAK_JSON_DIAG_POP
#endif

#endif // MERAK_JSON_FILESTREAM_H_
