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

#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <sys/time.h>
#include <time.h>
#include <typeinfo>
#include <limits>
#include <google/protobuf/descriptor.h>
#include <turbo/strings/numbers.h>
#include <merak/flatten/json_to_flat.h>
#include <merak/utility/zero_copy_stream_reader.h>       // ZeroCopyStreamReader
#include <merak/proto/encode_decode.h>
#include <turbo/strings/str_format.h>
#include <merak/proto/descriptor.h>
#include <merak/json.h>
#include <turbo/strings/escaping.h>
#include <turbo/log/logging.h>
#include <merak/proto/utility.h>


namespace merak {
    class JsonToFlatConverter {
    public:
        explicit JsonToFlatConverter(const Json2FlatOptions &opt) : _options(opt) {
        }

        bool convert(const merak::json::Value &json, FlatHandlerBase &handler, const merak::json::Value* root_val);

        [[nodiscard]] const std::string &ErrorText() const { return _error; }

        const std::string &error() const { return _error; }
    private:

        void start_object(const std::string &name) {
            _paths.push_back(name);
            _current_path = turbo::str_join(_paths, ".");
        }

        void end_object() {
            auto b = _paths.back();

            _paths.pop_back();
            _current_path = turbo::str_join(_paths, ".");
        }

        std::string make_key(std::string_view name) {
            if (name.empty()) {
                return _current_path;
            }
            return _current_path + "." + std::string(name);
        }

        std::string _error;
        Json2FlatOptions _options;
        std::vector<std::string> _paths;
        std::string _current_path;
    };


    bool JsonToFlatConverter::convert(const merak::json::Value &json_value, FlatHandlerBase &handler, const merak::json::Value* root_val) {
        if (!root_val) {
            if (!json_value.is_object()) {
                json_to_pb_error(&_error, "Root json must be an object");
                return false;
            }
            handler.start_object();
        }
        if (json_value.is_array()) {
            if (root_val && root_val->member_count() == 1) {
                for (auto i = 0; i < json_value.size(); ++i) {
                    start_object(turbo::str_format("[%d]",i));
                    if (!convert(json_value[i], handler,&json_value)) {
                        return false;
                    }
                    end_object();
                }
                return true;
            }

            json_to_pb_error(&_error, "the input json can't be array here");
            return false;
        }

        if (json_value.is_bool()) {
            std::string key = make_key("");
            handler.emplace_key(key.data(), key.size(), true);
            handler.emplace_bool(json_value.get_bool());
        } else if (json_value.is_string()) {
            std::string key = make_key("");
            handler.emplace_key(key.data(), key.size(), true);

            handler.emplace_string(json_value.get_string(), json_value.get_string_length(), false);
        }
        else if (json_value.is_int32()) {
            std::string key = make_key("");
            handler.emplace_key(key.data(), key.size(), true);
            handler.emplace_int32(json_value.get_int32());
        }
        else if (json_value.is_int64()) {
            std::string key = make_key("");
            handler.emplace_key(key.data(), key.size(), true);
            handler.emplace_int64(json_value.get_int64());
        }else if (json_value.is_uint32()) {
            std::string key = make_key("");
            handler.emplace_key(key.data(), key.size(), true);
            handler.emplace_uint32(json_value.get_uint32());
        }
        else if (json_value.is_uint64()) {
            std::string key = make_key("");
            handler.emplace_key(key.data(), key.size(), true);
            handler.emplace_uint64(json_value.get_uint64());
        } else if (json_value.is_float()) {
            std::string key = make_key("");
            handler.emplace_key(key.data(), key.size(), true);
            handler.emplace_double(json_value.get_float());
        }else if (json_value.is_double()) {
            std::string key = make_key("");
            handler.emplace_key(key.data(), key.size(), true);
            handler.emplace_double(json_value.get_double());
        }else if (json_value.is_object()) {
           for (auto it = json_value.member_begin(); it != json_value.member_end(); ++it) {
                start_object(it->name.get_string());
               if (!convert(it->value, handler, &json_value)) {
                   return false;
               }
               end_object();
           }
        }

        if (!root_val) {
            handler.end_object();
        }
        return true;
    }

    inline turbo::Status json_to_flat_inline(const std::string &json_string,
                                             turbo::Nonnull<FlatHandlerBase *> handler,
                                             const Json2FlatOptions &options,
                                             size_t *parsed_offset) {
        merak::json::Document d;
        if (options.allow_remaining_bytes_after_parsing) {
            d.parse<merak::json::kParseStopWhenDoneFlag>(json_string.c_str());
            if (parsed_offset != nullptr) {
                *parsed_offset = d.get_error_offset();
            }
        } else {
            d.parse<0>(json_string.c_str());
        }
        if (d.has_parse_error()) {
            if (options.allow_remaining_bytes_after_parsing) {
                if (d.get_parse_error() == merak::json::kParseErrorDocumentEmpty) {
                    // This is usual when parsing multiple jsons, don't waste time
                    // on setting the `empty error'
                    return turbo::not_found_error("");
                }
            }

            return turbo::invalid_argument_error("Invalid json: ",
                                                 merak::json::get_parse_error_en(d.get_parse_error()));
        }
        JsonToFlatConverter cc(options);
        auto r = cc.convert(d, *handler,  nullptr);
        if (TURBO_UNLIKELY(!r)) {
            return turbo::invalid_argument_error(cc.error());
        }
        return turbo::OkStatus();
    }

    turbo::Status json_to_flat(const merak::json::Value &json,
                               FlatHandlerBase *handler,
                               const Json2FlatOptions &options) {
        std::string err;
        JsonToFlatConverter cc(options);
        auto r = cc.convert(json, *handler, nullptr);
        if (TURBO_UNLIKELY(!r)) {
            return turbo::invalid_argument_error(err);
        }
        return turbo::OkStatus();
    }

    turbo::Status json_to_flat(const std::string &json_string,
                               turbo::Nonnull<FlatHandlerBase *> message,
                               const Json2FlatOptions &options,
                               size_t *parsed_offset) {
        return json_to_flat_inline(json_string, message, options, parsed_offset);
    }

    turbo::Status json_to_flat(google::protobuf::io::ZeroCopyInputStream *stream,
                               turbo::Nonnull<FlatHandlerBase *> message,
                               const Json2FlatOptions &options,
                               size_t *parsed_offset) {
        ZeroCopyStreamReader reader(stream);
        return json_to_flat(&reader, message, options, parsed_offset);
    }

    turbo::Status json_to_flat(ZeroCopyStreamReader *reader,
                               turbo::Nonnull<FlatHandlerBase *> handler,
                               const Json2FlatOptions &options,
                               size_t *parsed_offset) {
        merak::json::Document d;
        if (options.allow_remaining_bytes_after_parsing) {
            d.parse_stream<merak::json::kParseStopWhenDoneFlag, merak::json::UTF8<> >(
                *reader);
            if (parsed_offset != nullptr) {
                *parsed_offset = d.get_error_offset();
            }
        } else {
            d.parse_stream<0, merak::json::UTF8<> >(*reader);
        }
        if (d.has_parse_error()) {
            if (options.allow_remaining_bytes_after_parsing) {
                if (d.get_parse_error() == merak::json::kParseErrorDocumentEmpty) {
                    // This is usual when parsing multiple jsons, don't waste time
                    // on setting the `empty error'
                    return turbo::not_found_error();
                }
            }
            return turbo::invalid_argument_error("Invalid json: ",
                                                 merak::json::get_parse_error_en(d.get_parse_error()));
        }
        JsonToFlatConverter cc(options);
        auto r = cc.convert(d, *handler,  nullptr);
        if (TURBO_UNLIKELY(!r)) {
            return turbo::invalid_argument_error(cc.error());
        }
        return turbo::OkStatus();
    }

    turbo::Status json_to_flat(const std::string &json_string,
                               turbo::Nonnull<FlatHandlerBase *> message) {
        return json_to_flat_inline(json_string, message, Json2FlatOptions(), nullptr);
    }


    turbo::Status json_to_flat(google::protobuf::io::ZeroCopyInputStream *stream,
                               turbo::Nonnull<FlatHandlerBase *> message) {
        return json_to_flat(stream, message, Json2FlatOptions(), nullptr);
    }

} //namespace merak
