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

#ifndef MERAK_JSON_CURSORSTREAMWRAPPER_H_
#define MERAK_JSON_CURSORSTREAMWRAPPER_H_

#include <merak/json/stream.h>

#if defined(__GNUC__)
MERAK_JSON_DIAG_PUSH
MERAK_JSON_DIAG_OFF(effc++)
#endif

#if defined(_MSC_VER) && _MSC_VER <= 1800
MERAK_JSON_DIAG_PUSH
MERAK_JSON_DIAG_OFF(4702)  // unreachable code
MERAK_JSON_DIAG_OFF(4512)  // assignment operator could not be generated
#endif

namespace merak::json {


//! Cursor stream wrapper for counting line and column number if error exists.
/*!
    \tparam InputStream     Any stream that implements Stream Concept
*/
template <typename InputStream, typename Encoding = UTF8<> >
class CursorStreamWrapper : public GenericStreamWrapper<InputStream, Encoding> {
public:
    typedef typename Encoding::char_type char_type;

    CursorStreamWrapper(InputStream& is):
        GenericStreamWrapper<InputStream, Encoding>(is), line_(1), col_(0) {}

    // counting line and column number
    char_type get() {
        char_type ch = this->is_.get();
        if(ch == '\n') {
            line_ ++;
            col_ = 0;
        } else {
            col_ ++;
        }
        return ch;
    }

    //! Get the error line number, if error exists.
    size_t GetLine() const { return line_; }
    //! Get the error column number, if error exists.
    size_t GetColumn() const { return col_; }

private:
    size_t line_;   //!< Current Line
    size_t col_;    //!< Current Column
};

#if defined(_MSC_VER) && _MSC_VER <= 1800
MERAK_JSON_DIAG_POP
#endif

#if defined(__GNUC__)
MERAK_JSON_DIAG_POP
#endif

}  // namespace merak::json

#endif // MERAK_JSON_CURSORSTREAMWRAPPER_H_
