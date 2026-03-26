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
#include <merak/json/std_input_stream.h>
#include <merak/json/stringbuffer.h>
#include <merak/json/writer.h>
#include <merak/json/prettywriter.h>
#include <merak/json/document_internal.h>
#include <turbo/utility/status.h>
#include <turbo/files/filesystem.h>
#include <vector>

namespace merak::json {

    template<unsigned parseFlag = kParseDefaultFlags, typename SourceEncoding = UTF8<>, typename DocEncoding = UTF8<>, typename Allocator = MERAK_JSON_DEFAULT_ALLOCATOR, typename StackAllocator = MERAK_JSON_DEFAULT_STACK_ALLOCATOR>
    inline GenericDocument<DocEncoding, Allocator, StackAllocator>
    parse_cstr(const typename SourceEncoding::char_type *str) noexcept {
        GenericDocument < DocEncoding, Allocator > doc;
        doc.template parse<parseFlag, SourceEncoding>(str);
        return std::move(doc);
    }

    template<unsigned parseFlag = kParseDefaultFlags, typename SourceEncoding = UTF8<>, typename DocEncoding = UTF8<>, typename Allocator = MERAK_JSON_DEFAULT_ALLOCATOR, typename StackAllocator = MERAK_JSON_DEFAULT_STACK_ALLOCATOR>
    inline void
    parse_cstr(const typename SourceEncoding::char_type *str,
               GenericDocument<DocEncoding, Allocator, StackAllocator> &doc) noexcept {
        doc.template parse<parseFlag, SourceEncoding>(str);
    }

    template<unsigned parseFlag = kParseStopWhenDoneFlag, typename SourceEncoding = UTF8<>, typename DocEncoding = UTF8<>, typename Allocator = MERAK_JSON_DEFAULT_ALLOCATOR, typename StackAllocator = MERAK_JSON_DEFAULT_STACK_ALLOCATOR>
    const typename SourceEncoding::char_type *parse_cstr_partial(const typename SourceEncoding::char_type *str,
                                                                 GenericDocument<DocEncoding, Allocator, StackAllocator> &doc) noexcept {
        doc.template parse<parseFlag, SourceEncoding>(str);
        auto offset = doc.get_error_offset();
        if (doc.get_parse_error() == kParseErrorDocumentEmpty) {
            return nullptr;
        }
        return str + offset;
    }

    template<unsigned parseFlag = kParseStopWhenDoneFlag, typename SourceEncoding = UTF8<>, typename DocEncoding = UTF8<>, typename Allocator = MERAK_JSON_DEFAULT_ALLOCATOR, typename StackAllocator = MERAK_JSON_DEFAULT_STACK_ALLOCATOR>
    ParseResult parse_cstr_all(const typename SourceEncoding::char_type *str,
                               std::vector<GenericDocument<DocEncoding, Allocator, StackAllocator>> &docs) noexcept {
        ParseResult result;
        size_t total_parsed = 0;
        while (str) {
            GenericDocument < DocEncoding, Allocator, StackAllocator > doc;
            doc.template parse<parseFlag, SourceEncoding>(str);
            ParseResult err = doc;
            total_parsed += err.Offset();
            if (doc.has_parse_error()) {
                if (doc.get_parse_error() == kParseErrorDocumentEmpty) {
                    result.set(kParseErrorNone, total_parsed);
                } else {
                    result.set(doc.get_parse_error(), total_parsed);
                }
                return result;
            }
            docs.push_back(std::move(doc));
            str = str + err.Offset();
        }
        result.set(kParseErrorNone, total_parsed);
        return result;
    }

    template<unsigned parseFlag = kParseDefaultFlags, typename SourceEncoding = UTF8<>, typename DocEncoding = UTF8<>, typename Allocator = MERAK_JSON_DEFAULT_ALLOCATOR, typename StackAllocator = MERAK_JSON_DEFAULT_STACK_ALLOCATOR>
    GenericDocument<DocEncoding, Allocator, StackAllocator>
    parse_array(std::basic_string_view<typename SourceEncoding::char_type> str) noexcept {
        GenericDocument < DocEncoding, Allocator > doc;
        doc.template parse<parseFlag, SourceEncoding>(str.data(), str.size());
        return std::move(doc);
    }

    template<unsigned parseFlag = kParseDefaultFlags, typename SourceEncoding = UTF8<>, typename DocEncoding = UTF8<>, typename Allocator = MERAK_JSON_DEFAULT_ALLOCATOR, typename StackAllocator = MERAK_JSON_DEFAULT_STACK_ALLOCATOR>
    inline void parse_array(std::basic_string_view<typename SourceEncoding::char_type> str,
                            GenericDocument<DocEncoding, Allocator, StackAllocator> &doc) noexcept {
        doc.template parse<parseFlag, SourceEncoding>(str.data(), str.size());
    }

    template<unsigned parseFlag = kParseStopWhenDoneFlag, typename SourceEncoding = UTF8<>, typename DocEncoding = UTF8<>, typename Allocator = MERAK_JSON_DEFAULT_ALLOCATOR, typename StackAllocator = MERAK_JSON_DEFAULT_STACK_ALLOCATOR>
    std::basic_string_view<typename SourceEncoding::char_type>
    parse_array_partial(std::basic_string_view<typename SourceEncoding::char_type> str,
                        GenericDocument<DocEncoding, Allocator, StackAllocator> &doc) noexcept {
        doc.template parse<parseFlag, SourceEncoding>(str.data(), str.size());
        auto offset = doc.get_error_offset();
        if (doc.get_parse_error() == kParseErrorDocumentEmpty) {
            return std::basic_string_view<typename SourceEncoding::char_type>();
        }
        return str.substr(offset);
    }

    template<unsigned parseFlag = kParseStopWhenDoneFlag, typename SourceEncoding = UTF8<>, typename DocEncoding = UTF8<>, typename Allocator = MERAK_JSON_DEFAULT_ALLOCATOR, typename StackAllocator = MERAK_JSON_DEFAULT_STACK_ALLOCATOR>
    ParseResult parse_array_all(std::basic_string_view<typename SourceEncoding::char_type> str,
                                std::vector<GenericDocument<DocEncoding, Allocator, StackAllocator>> &docs) noexcept {
        ParseResult result;
        size_t total_parsed = 0;
        while (!str.empty()) {
            GenericDocument < DocEncoding, Allocator, StackAllocator > doc;
            doc.template parse<parseFlag, SourceEncoding>(str.data(), str.size());
            ParseResult err = doc;
            total_parsed += err.Offset();
            if (doc.has_parse_error()) {
                if (doc.get_parse_error() == kParseErrorDocumentEmpty) {
                    result.set(kParseErrorNone, total_parsed);
                } else {
                    result.set(doc.get_parse_error(), total_parsed);
                }
                return result;
            }
            docs.push_back(std::move(doc));
            str = str.substr(err.Offset());
        }
        result.set(kParseErrorNone, total_parsed);
        return result;
    }

    template<unsigned parseFlag = kParseDefaultFlags, typename SourceEncoding = UTF8<>, typename DocEncoding = UTF8<>, typename Allocator = MERAK_JSON_DEFAULT_ALLOCATOR, typename StackAllocator = MERAK_JSON_DEFAULT_STACK_ALLOCATOR>
    GenericDocument<DocEncoding, Allocator, StackAllocator>
    parse_stream(std::basic_istream<typename SourceEncoding::char_type> &is) noexcept {
        GenericDocument < DocEncoding, Allocator > doc;
        BasicStdInputStream iis(is);
        doc.template parse_stream<parseFlag, SourceEncoding>(iis);
        return std::move(doc);
    }

    template<unsigned parseFlag = kParseDefaultFlags, typename SourceEncoding = UTF8<>, typename DocEncoding = UTF8<>, typename Allocator = MERAK_JSON_DEFAULT_ALLOCATOR, typename StackAllocator = MERAK_JSON_DEFAULT_STACK_ALLOCATOR>
    inline void parse_stream(std::basic_istream<typename SourceEncoding::char_type> &is,
                             GenericDocument<DocEncoding, Allocator, StackAllocator> &doc) noexcept {
        BasicStdInputStream iis(is);
        doc.template parse_stream<parseFlag, SourceEncoding>(iis);
    }

    template<unsigned parseFlag = kParseStopWhenDoneFlag, typename SourceEncoding = UTF8<>, typename DocEncoding = UTF8<>, typename Allocator = MERAK_JSON_DEFAULT_ALLOCATOR, typename StackAllocator = MERAK_JSON_DEFAULT_STACK_ALLOCATOR>
    ParseResult parse_stream_partial(std::basic_istream<typename SourceEncoding::char_type> &is,
                                     GenericDocument<DocEncoding, Allocator, StackAllocator> &doc) noexcept {
        BasicStdInputStream iis(is);
        doc.template parse_stream<parseFlag, SourceEncoding>(iis);
        ParseResult err = doc;
        return err;
    }

    template<unsigned parseFlag = kParseStopWhenDoneFlag, typename SourceEncoding = UTF8<>, typename DocEncoding = UTF8<>, typename Allocator = MERAK_JSON_DEFAULT_ALLOCATOR, typename StackAllocator = MERAK_JSON_DEFAULT_STACK_ALLOCATOR>
    ParseResult parse_stream_all(std::basic_istream<typename SourceEncoding::char_type> &is,
                                 std::vector<GenericDocument<DocEncoding, Allocator, StackAllocator>> &docs) noexcept {
        BasicStdInputStream iis(is);
        ParseResult result;
        size_t total_parsed = 0;
        while (!result.IsError()) {
            GenericDocument < DocEncoding, Allocator > doc;
            doc.template parse_stream<parseFlag, SourceEncoding>(iis);
            ParseResult err = doc;
            total_parsed += err.Offset();
            if (doc.has_parse_error()) {
                if (doc.get_parse_error() == kParseErrorDocumentEmpty) {
                    result.set(kParseErrorNone, total_parsed);
                } else {
                    result.set(doc.get_parse_error(), total_parsed);
                }
                return result;
            }
            docs.push_back(std::move(doc));
        }
        return result;
    }

    template<typename InputStream, unsigned parseFlag = kParseDefaultFlags, typename SourceEncoding = UTF8<>, typename DocEncoding = UTF8<>, typename Allocator = MERAK_JSON_DEFAULT_ALLOCATOR, typename StackAllocator = MERAK_JSON_DEFAULT_STACK_ALLOCATOR>
    GenericDocument<DocEncoding, Allocator, StackAllocator>
    parse_stream(InputStream &iis) noexcept {
        GenericDocument < DocEncoding, Allocator > doc;
        doc.template parse_stream<parseFlag, SourceEncoding>(iis);
        return std::move(doc);
    }

    template<typename InputStream, unsigned parseFlag = kParseDefaultFlags, typename SourceEncoding = UTF8<>, typename DocEncoding = UTF8<>, typename Allocator = MERAK_JSON_DEFAULT_ALLOCATOR, typename StackAllocator = MERAK_JSON_DEFAULT_STACK_ALLOCATOR>
    inline void parse_stream(InputStream &iis, GenericDocument<DocEncoding, Allocator, StackAllocator> &doc) noexcept {
        doc.template parse_stream<parseFlag, SourceEncoding>(iis);
    }

    template<typename InputStream, unsigned parseFlag = kParseStopWhenDoneFlag, typename SourceEncoding = UTF8<>, typename DocEncoding = UTF8<>, typename Allocator = MERAK_JSON_DEFAULT_ALLOCATOR, typename StackAllocator = MERAK_JSON_DEFAULT_STACK_ALLOCATOR>
    ParseResult parse_stream_partial(InputStream &is,
                                     GenericDocument<DocEncoding, Allocator, StackAllocator> &doc) noexcept {
        doc.template parse_stream<parseFlag, SourceEncoding>(is);
        ParseResult err = doc;
        return err;
    }

    template<typename InputStream, unsigned parseFlag = kParseStopWhenDoneFlag, typename SourceEncoding = UTF8<>, typename DocEncoding = UTF8<>, typename Allocator = MERAK_JSON_DEFAULT_ALLOCATOR, typename StackAllocator = MERAK_JSON_DEFAULT_STACK_ALLOCATOR>
    ParseResult parse_stream_all(InputStream &is,
                                 std::vector<GenericDocument<DocEncoding, Allocator, StackAllocator>> &docs) noexcept {
        ParseResult result;
        size_t total_parsed = 0;
        while (!result.IsError()) {
            GenericDocument < DocEncoding, Allocator > doc;
            doc.template parse_stream<parseFlag, SourceEncoding>(is);
            ParseResult err = doc;
            total_parsed += err.Offset();
            if (doc.has_parse_error()) {
                if (doc.get_parse_error() == kParseErrorDocumentEmpty) {
                    result.set(kParseErrorNone, total_parsed);
                } else {
                    result.set(doc.get_parse_error(), total_parsed);
                }
                return result;
            }
            docs.push_back(std::move(doc));
        }
        return result;
    }
    template<typename SourceEncoding = UTF8<>>
    turbo::Result<std::basic_ifstream<typename SourceEncoding::char_type>> create_file_stream(const turbo::FilePath &path) {
        std::basic_ifstream<typename SourceEncoding::char_type> fs;
        fs.open(path.string(), std::ios_base::in | std::ios_base::binary);
        if (!fs.is_open()) {
            return turbo::io_error_with_errno_payload(errno, "open file error: ", path.string());
        }
        return std::move(fs);
    }

}  // namespace merak::json
