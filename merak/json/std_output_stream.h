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
