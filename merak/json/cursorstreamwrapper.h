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
