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

#include <merak/utility/zero_copy_stream_reader.h>
#include <merak/json.h>
#include <turbo/utility/status.h>
#include <google/protobuf/message.h>
#include <google/protobuf/io/zero_copy_stream.h>    // ZeroCopyInputStream
#include <merak/flat_handler.h>
#include <merak/options.h>

namespace merak {

    // Convert `json' to flat json `message'.
    // Returns true on success. `error' (if not nullptr) will be set with error
    // message on failure.
    //
    // [When options.allow_remaining_bytes_after_parsing is true]
    // * `parse_offset' will be set with #bytes parsed
    // * the function still returns false on empty document but the `error' is set
    //   to empty string instead of `The document is empty'.
    turbo::Status json_to_flat(const std::string& json,
                            turbo::Nonnull<FlatHandlerBase*> message,
                            const Json2FlatOptions& options,
                            size_t* parsed_offset = nullptr);

    // Use ZeroCopyInputStream as input instead of std::string.
    turbo::Status json_to_flat(google::protobuf::io::ZeroCopyInputStream *json,
                                        turbo::Nonnull<FlatHandlerBase*> message,
                            const Json2FlatOptions &options,
                            size_t *parsed_offset = nullptr);

    turbo::Status json_to_flat(const merak::json::Value &json,
                                        turbo::Nonnull<FlatHandlerBase*> message,
                               const Json2FlatOptions &options = Json2FlatOptions());


    // Use ZeroCopyStreamReader as input instead of std::string.
    // If you need to parse multiple jsons from IOBuf, you should use this
    // overload instead of the ZeroCopyInputStream one which bases on this
    // and recreates a ZeroCopyStreamReader internally that can't be reused
    // between continuous calls.
    turbo::Status json_to_flat(ZeroCopyStreamReader *json,
                                        turbo::Nonnull<FlatHandlerBase*> message,
                            const Json2FlatOptions& options,
                            size_t* parsed_offset = nullptr);

    // Using default Json2PbOptions.
    turbo::Status json_to_flat(const std::string& json,
                                        turbo::Nonnull<FlatHandlerBase*> message);

    turbo::Status json_to_flat(google::protobuf::io::ZeroCopyInputStream* stream,
                                        turbo::Nonnull<FlatHandlerBase*> message);

    // Using default Pb2JsonOptions.
    template<typename T>
    turbo::Status json_to_flat_json(T message, std::string *json,
                                    const Json2FlatOptions &options = Json2FlatOptions(), size_t* parsed_offset = nullptr) {
        merak::json::StringBuffer buffer;
        merak::json::PrettyWriter<merak::json::StringBuffer> writer(buffer);
        FlatHandlerTemplate<merak::json::PrettyWriter<merak::json::StringBuffer>> handler(writer);
        TURBO_RETURN_NOT_OK(merak::json_to_flat(message,&handler, options, parsed_offset));
        json->append(buffer.get_string(), buffer.GetSize());
        return turbo::OkStatus();
    }

    // Using default Pb2JsonOptions.
    template<typename T>
    turbo::Status json_to_flat_map(T message, turbo::flat_hash_map<std::string, PrimitiveValue> &result,
                                    const Json2FlatOptions &options = Json2FlatOptions(), size_t* parsed_offset = nullptr) {
        FlatContainerHandler<turbo::flat_hash_map<std::string, PrimitiveValue>> handler(result);
        TURBO_RETURN_NOT_OK(merak::json_to_flat(message,&handler, options, parsed_offset));
        return turbo::OkStatus();
    }

    // Using default Pb2JsonOptions.
    template<typename T>
    turbo::Status json_to_flat_map(T message, turbo::flat_hash_map<std::string, std::string> &result,
                                    const Json2FlatOptions &options = Json2FlatOptions(), size_t* parsed_offset = nullptr) {
        FlatContainerHandler<turbo::flat_hash_map<std::string, std::string>>  handler(result);
        TURBO_RETURN_NOT_OK(merak::json_to_flat(message,&handler, options, parsed_offset));
        return turbo::OkStatus();
    }


} // namespace merak
