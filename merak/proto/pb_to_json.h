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

namespace merak {

    enum EnumOption {
        OUTPUT_ENUM_BY_NAME = 0,          // Output enum by its name
        OUTPUT_ENUM_BY_NUMBER = 1,        // Output enum by its value
    };

    struct Pb2JsonOptions {
        // Control how enum fields are output
        // Default: OUTPUT_ENUM_BY_NAME
        EnumOption enum_option{OUTPUT_ENUM_BY_NAME};

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

    // Convert protobuf `messge' to `json' according to `options'.
    // Returns true on success. `error' (if not nullptr) will be set with error
    // message on failure.
    turbo::Status proto_message_to_json(const google::protobuf::Message& message,
                            std::string* json,
                            const Pb2JsonOptions& options);

    // send output to ZeroCopyOutputStream instead of std::string.
    turbo::Status proto_message_to_json(const google::protobuf::Message& message,
                            google::protobuf::io::ZeroCopyOutputStream *json,
                            const Pb2JsonOptions& options);

    // Using default Pb2JsonOptions.
    turbo::Status proto_message_to_json(const google::protobuf::Message& message,
                            std::string* json);

    turbo::Status proto_message_to_json(const google::protobuf::Message& message,
                            google::protobuf::io::ZeroCopyOutputStream* json);

    turbo::Status proto_message_to_json(const google::protobuf::Message& message,
                                        merak::json::Document &json);

    turbo::Status proto_message_to_json(const google::protobuf::Message& message,
                                        merak::json::Document &json, const Pb2JsonOptions& options);

} // namespace merak
