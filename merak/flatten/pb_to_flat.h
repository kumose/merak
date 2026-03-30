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

#pragma once


#include <string>
#include <turbo/utility/status.h>
#include <google/protobuf/message.h>
#include <google/protobuf/io/zero_copy_stream.h> // ZeroCopyOutputStream
#include <merak/json.h>
#include <turbo/container/flat_hash_map.h>

namespace merak {
    enum FlatEnumOption {
        OUTPUT_ENUM_BY_NAME = 0, // Output enum by its name
        OUTPUT_ENUM_BY_NUMBER = 1, // Output enum by its value
    };

    struct Pb2FlatOptions {
        // Control how enum fields are output
        // Default: OUTPUT_ENUM_BY_NAME
        FlatEnumOption enum_option{OUTPUT_ENUM_BY_NAME};

        // Use merak::json::PrettyWriter to generate the json when this option is on.
        // NOTE: currently PrettyWriter is not optimized yet thus the conversion
        // functions may be slower when this option is turned on.
        // Default: false
        bool pretty_json{false};

        // Convert "repeated { required string key = 1; required string value = 2; }"
        // to a map object of json and vice versa when this option is turned on.
        // Default: true
        bool enable_protobuf_map{true};

        // Encode the field of type bytes to string in json using base64
        // encoding when this option is turned on.
        // Default: false for baidu-internal, true otherwise.
        bool bytes_to_base64{true};

        // Convert the repeated field that has no entry
        // to a empty array of json when this option is turned on.
        // Default: false
        bool jsonify_empty_array{false};

        // Whether to always print primitive fields. By default proto3 primitive
        // fields with default values will be omitted in JSON output. For example, an
        // int32 field set to 0 will be omitted. Set this flag to true will override
        // the default behavior and print primitive fields regardless of their values.
        bool always_print_primitive_fields{false};

        // Convert the single repeated field to a json array when this option is turned on.
        // Default: false.
        bool single_repeated_to_array{false};

        // using `@type` instead of `type_url`
        // for Any in json field
        bool using_a_type_url{false};
    };

    // Using default Pb2JsonOptions.
    void proto_message_to_flat_json(const google::protobuf::Message &message, std::string *json,
                                    const Pb2FlatOptions &op = Pb2FlatOptions());

    using FlatValueType = std::variant<std::string, bool, int32_t, uint32_t, int64_t, uint64_t, float, double>;
    using FlatValueMap = turbo::flat_hash_map<std::string, FlatValueType>;



    class FlatHandler {
    public:
        virtual ~FlatHandler() = default;

        virtual void start_object() = 0;

        virtual void end_object(int n = 0) = 0;

        virtual void emplace_key(const char *data, size_t size, bool copy) = 0;

        virtual void emplace_bool(bool value) = 0;

        virtual void emplace_int32(int32_t value) = 0;

        virtual void emplace_uint32(uint32_t value) = 0;

        virtual void emplace_int64(int64_t value) = 0;

        virtual void emplace_uint64(uint64_t value) = 0;

        virtual void emplace_double(double value) = 0;

        virtual void emplace_string(const char *data, size_t len, bool) = 0;
    };

    class FlatMapHandler : public FlatHandler {
    public:
        FlatMapHandler() = default;

        ~FlatMapHandler() = default;

        void start_object() {
        }

        void end_object(int n = 0) {
        }

        void emplace_key(const char *data, size_t size, bool copy) {
            if (!copy) {
                key = std::string_view(data, size);
                return;
            }
            _key.assign(data, size);
            key = _key;
        }

        void emplace_bool(bool value) {
            FlatValueType v = value;
            flatmap[key] = std::move(v);
        }

        void emplace_int32(int32_t value) {
            FlatValueType v = value;
            flatmap[key] = std::move(v);
        }

        void emplace_uint32(uint32_t value) {
            FlatValueType v = value;
            flatmap[key] = std::move(v);
        }

        void emplace_int64(int64_t value) {
            FlatValueType v = value;
            flatmap[key] = std::move(v);
        }

        void emplace_uint64(uint64_t value) {
            FlatValueType v = value;
            flatmap[key] = std::move(v);
        }

        void emplace_double(double value) {
            FlatValueType v = value;
            flatmap[key] = std::move(v);
        }

        void emplace_string(const char *data, size_t len, bool) {
            FlatValueType value;
            value = std::string(data, len);
            flatmap[key] = std::move(value);
        }

        turbo::flat_hash_map<std::string, FlatValueType> flatmap;

        /// no need to copy
        std::string_view key;
        std::string _key;
    };

    class FlatStringMapHandler : public FlatHandler  {
    public:
        FlatStringMapHandler() = default;

        ~FlatStringMapHandler() = default;

        void start_object() {
        }

        void end_object(int n = 0) {
        }

        void emplace_key(const char *data, size_t size, bool copy) {
            if (!copy) {
                key = std::string_view(data, size);
                return;
            }
            _key.assign(data, size);
            key = _key;
        }

        void emplace_bool(bool value) {
            if (value)
                flatmap[key] = "true";
            else
                flatmap[key] = "false";
        }

        void emplace_int32(int32_t value) {
            flatmap[key] = turbo::str_cat(value);
        }

        void emplace_uint32(uint32_t value) {
            flatmap[key] = turbo::str_cat(value);
        }

        void emplace_int64(int64_t value) {
            flatmap[key] = turbo::str_cat(value);
        }

        void emplace_uint64(uint64_t value) {
            flatmap[key] = turbo::str_cat(value);
        }

        void emplace_double(double value) {
            flatmap[key] = turbo::str_cat(value);
        }

        void emplace_string(const char *data, size_t len, bool) {
            flatmap[key] = std::move(std::string(data, len));
        }

        turbo::flat_hash_map<std::string, std::string> flatmap;

        /// no need to copy
        std::string_view key;
        std::string _key;
    };


    void proto_message_to_flat(const google::protobuf::Message &message,
                           turbo::flat_hash_map<std::string, FlatValueType> &result,
                           const Pb2FlatOptions &op = Pb2FlatOptions());

    void proto_message_to_flat(const google::protobuf::Message &message,
                               turbo::flat_hash_map<std::string, std::string> &result,
                               const Pb2FlatOptions &op = Pb2FlatOptions());

    void proto_message_to_flat(const google::protobuf::Message &message,
                               FlatHandler &result,
                               const Pb2FlatOptions &op = Pb2FlatOptions());


    inline std::ostream &operator<<(std::ostream &os,
                                    const turbo::flat_hash_map<std::string, FlatValueType> &flat_map) {
        for (const auto &kv: flat_map) {
            os << kv.first << "=";

            std::visit([&os](auto &&arg) {
                using T = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<T, std::string>) {
                    os << arg;
                }
                // bool  true / false
                else if constexpr (std::is_same_v<T, bool>) {
                    os << std::boolalpha << arg;
                } else {
                    os << turbo::str_cat(arg);
                }
            }, kv.second);

            os << "\n";
        }
        return os;
    }

    inline std::ostream &operator<<(std::ostream &os, const turbo::flat_hash_map<std::string, std::string> &flat_map) {
        for (const auto &kv: flat_map) {
            os << kv.first << "=" << kv.second << "\n";
        }
        return os;
    }
} // namespace merak
