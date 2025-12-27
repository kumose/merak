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

#ifndef MERAK_JSON_OSTREAMWRAPPER_H_
#define MERAK_JSON_OSTREAMWRAPPER_H_

#include <merak/json/stream.h>
#include <iosfwd>
#include <ostream>

#ifdef __clang__
MERAK_JSON_DIAG_PUSH
MERAK_JSON_DIAG_OFF(padded)
#endif

namespace merak::json {

    //! Wrapper of \c std::basic_ostream into RapidJSON's Stream concept.
    /*!
        The classes can be wrapped including but not limited to:

        - \c std::ostringstream
        - \c std::stringstream
        - \c std::wpstringstream
        - \c std::wstringstream
        - \c std::ifstream
        - \c std::fstream
        - \c std::wofstream
        - \c std::wfstream

        \tparam StreamType Class derived from \c std::basic_ostream.
    */

    template<typename StreamType>
    class BasicStdOutputStream {
    public:
        typedef typename StreamType::char_type char_type;

        BasicStdOutputStream(StreamType &stream) : stream_(stream) {}

        void put(char_type c) {
            stream_.put(c);
        }

        void write(const char_type* c, size_t length) {
            stream_.write(c,length);
        }

        BasicStdOutputStream& flush() {
            stream_.flush();
            return *this;
        }

    private:
        BasicStdOutputStream(const BasicStdOutputStream &);

        BasicStdOutputStream &operator=(const BasicStdOutputStream &);

        StreamType &stream_;
    };

    using StdOutputStream =  BasicStdOutputStream<std::ostream>;
    using WStdOutputStream = BasicStdOutputStream<std::wostream>;

#ifdef __clang__
    MERAK_JSON_DIAG_POP
#endif

}  // namespace merak::json

#endif // MERAK_JSON_OSTREAMWRAPPER_H_
