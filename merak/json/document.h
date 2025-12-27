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

#pragma once

#include <merak/json/std_output_stream.h>
#include <merak/json/stringbuffer.h>
#include <merak/json/writer.h>
#include <merak/json/prettywriter.h>
#include <merak/json/document_internal.h>
#include <merak/json/parser.h>
#include <vector>

namespace merak::json {

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template<typename Encoding, typename Allocator>
    void GenericValue<Encoding, Allocator>::describe(std::ostream &os) {
        StdOutputStream writer_os(os);
        Writer<StdOutputStream, Encoding, UTF8<>> writer(writer_os);
        accept(writer);
    }

    template<typename Encoding, typename Allocator>
    [[nodiscard]] std::string
    GenericValue<Encoding, Allocator>::to_string() const {
        GenericStringBuffer<Encoding> sb;
        Writer<StringBuffer, Encoding, UTF8<>> writer(sb);
        accept(writer);
        std::string result(sb.get_string(), sb.GetSize());
        return std::move(result);
    }

    template<typename Encoding, typename Allocator>
    void GenericValue<Encoding, Allocator>::pretty_describe(std::ostream &os) {
        StdOutputStream writer_os(os);
        PrettyWriter<StdOutputStream, Encoding, UTF8<>> writer(writer_os);
        accept(writer);
    }

    template<typename Encoding, typename Allocator>
    [[nodiscard]] std::string GenericValue<Encoding, Allocator>::to_pretty_string() const {
        GenericStringBuffer<Encoding> sb;
        PrettyWriter<StringBuffer, Encoding, UTF8<>> writer(sb);
        accept(writer);
        std::string result(sb.get_string(), sb.GetSize());
        return std::move(result);
    }

}  // namespace merak::json
