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
#pragma once

#include <merak/utility/zero_copy_stream_reader.h>
#include <merak/json.h>
#include <turbo/utility/status.h>
#include <google/protobuf/message.h>
#include <google/protobuf/io/zero_copy_stream.h>    // ZeroCopyInputStream

namespace merak {

    struct Json2PbOptions {
        // Decode string in json using base64 decoding if the type of
        // corresponding field is bytes when this option is turned on.
        // Default: false for baidu-interal, true otherwise.
        bool base64_to_bytes{true};

        // Allow decoding json array iff there is only one repeated field.
        // Default: false.
        bool array_to_single_repeated{false};

        // Allow more bytes remaining in the input after parsing the first json
        // object. Useful when the input contains more than one json object.
        bool allow_remaining_bytes_after_parsing{false};

        // Allow compatible field name in json string.
        // eg define in proto optional string person_name,
        // it is compatible `personName` in json file name
        bool compatible_json_field{false};
    };

    // Convert `json' to protobuf `message'.
    // Returns true on success. `error' (if not nullptr) will be set with error
    // message on failure.
    //
    // [When options.allow_remaining_bytes_after_parsing is true]
    // * `parse_offset' will be set with #bytes parsed
    // * the function still returns false on empty document but the `error' is set
    //   to empty string instead of `The document is empty'.
    turbo::Status json_to_proto_message(const std::string& json,
                            turbo::Nonnull<google::protobuf::Message*> message,
                            const Json2PbOptions& options,
                            size_t* parsed_offset = nullptr);

    // Use ZeroCopyInputStream as input instead of std::string.
    turbo::Status json_to_proto_message(google::protobuf::io::ZeroCopyInputStream *json,
                                        turbo::Nonnull<google::protobuf::Message*> message,
                            const Json2PbOptions &options,
                            size_t *parsed_offset = nullptr);

    turbo::Status json_to_proto_message(const merak::json::Value &json,
                                        turbo::Nonnull<google::protobuf::Message*> message,
                               const Json2PbOptions &options = Json2PbOptions());


    // Use ZeroCopyStreamReader as input instead of std::string.
    // If you need to parse multiple jsons from IOBuf, you should use this
    // overload instead of the ZeroCopyInputStream one which bases on this
    // and recreates a ZeroCopyStreamReader internally that can't be reused
    // between continuous calls.
    turbo::Status json_to_proto_message(ZeroCopyStreamReader *json,
                                        turbo::Nonnull<google::protobuf::Message*> message,
                            const Json2PbOptions& options,
                            size_t* parsed_offset = nullptr);

    // Using default Json2PbOptions.
    turbo::Status json_to_proto_message(const std::string& json,
                                        turbo::Nonnull<google::protobuf::Message*> message);

    turbo::Status json_to_proto_message(google::protobuf::io::ZeroCopyInputStream* stream,
                                        turbo::Nonnull<google::protobuf::Message*> message);
} // namespace merak
