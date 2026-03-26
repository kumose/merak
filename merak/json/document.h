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
