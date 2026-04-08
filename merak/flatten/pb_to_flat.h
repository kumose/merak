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
#include <merak/flat_handler.h>
#include <merak/options.h>
#include <turbo/container/flat_hash_map.h>

namespace merak {

    // Using default Pb2JsonOptions.
    turbo::Status proto_message_to_flat_json(const google::protobuf::Message &message, std::string *json,
                                             const Pb2FlatOptions &op = Pb2FlatOptions());


    turbo::Status proto_message_to_flat(const google::protobuf::Message &message,
                                        turbo::flat_hash_map<std::string, PrimitiveValue> &result,
                                        const Pb2FlatOptions &op = Pb2FlatOptions());

    turbo::Status proto_message_to_flat(const google::protobuf::Message &message,
                                        turbo::flat_hash_map<std::string, std::string> &result,
                                        const Pb2FlatOptions &op = Pb2FlatOptions());

    turbo::Status proto_message_to_flat(const google::protobuf::Message &message,
                                        FlatHandlerBase &result,
                                        const Pb2FlatOptions &op = Pb2FlatOptions());

} // namespace merak
