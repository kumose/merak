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

#include <merak/flatten/pb_to_flat.h>

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <sys/time.h>
#include <time.h>
#include <google/protobuf/descriptor.h>
#include <merak/utility/zero_copy_stream_writer.h>
#include <merak/proto/encode_decode.h>
#include <merak/proto/descriptor.h>
#include <merak/json/std_output_stream.h>
#include <turbo/strings/string_builder.h>
#include <merak/json.h>
#include <turbo/strings/escaping.h>

namespace merak {

    class PbToFlatConverter {
    public:
        explicit PbToFlatConverter(const Pb2FlatOptions &opt) : _option(opt) {}

        template<typename Handler>
        turbo::Status Convert(const google::protobuf::Message &message, Handler &handler, bool root_msg = false);

        [[nodiscard]] const std::string &ErrorText() const { return _error; }

    private:
        template<typename Handler>
        turbo::Status pb_field_to_json(const google::protobuf::Message &message,
                              const google::protobuf::FieldDescriptor *field,
                              Handler &handler);
        template<typename Handler>
        turbo::Status pb_field_to_any_single(const google::protobuf::Message &message,
                              const google::protobuf::FieldDescriptor *field,
                              Handler &handler);

        template<typename Handler>
        turbo::Status pb_field_to_any(const google::protobuf::Message &message,
                                    const google::protobuf::FieldDescriptor *field,
                                    Handler &handler);

        void start_object(const std::string &name) {
            _paths.push_back(name);
            _current_path = turbo::str_join(_paths, ".");
        }

        void end_object() {
            auto b = _paths.back();

            _paths.pop_back();
            _current_path = turbo::str_join(_paths, ".");
        }

        std::string key(std::string_view name) {
            if (name.empty()) {
                return  _current_path;
            }
            return _current_path + "." + std::string(name);
        }
        std::string _error;
        Pb2FlatOptions _option;
        std::vector<std::string> _paths;
        std::string _current_path;
    };

    template<typename Handler>
    turbo::Status PbToFlatConverter::Convert(const google::protobuf::Message &message, Handler &handler, bool root_msg) {
        const google::protobuf::Reflection *reflection = message.GetReflection();
        const google::protobuf::Descriptor *descriptor = message.GetDescriptor();

        int ext_range_count = descriptor->extension_range_count();
        int field_count = descriptor->field_count();
        std::vector<const google::protobuf::FieldDescriptor *> fields;
        fields.reserve(64);
        for (int i = 0; i < ext_range_count; ++i) {
            const google::protobuf::Descriptor::ExtensionRange *
                    ext_range = descriptor->extension_range(i);
#if GOOGLE_PROTOBUF_VERSION < 4025000
            for (int tag_number = ext_range->start; tag_number < ext_range->end; ++tag_number)
#else
            for (int tag_number = ext_range->start_number(); tag_number < ext_range->end_number(); ++tag_number)
#endif
            {
                const google::protobuf::FieldDescriptor *field =
                        reflection->FindKnownExtensionByNumber(tag_number);
                if (field) {
                    fields.push_back(field);
                }
            }
        }
        std::vector<const google::protobuf::FieldDescriptor *> map_fields;
        for (int i = 0; i < field_count; ++i) {
            const google::protobuf::FieldDescriptor *field = descriptor->field(i);
            if (_option.enable_protobuf_map && merak::is_protobuf_map(field)) {
                map_fields.push_back(field);
            } else {
                fields.push_back(field);
            }
        }

        if (root_msg) {
            handler.start_object();
        }


        // Fill in non-map fields
        std::string field_name_str;
        for (size_t i = 0; i < fields.size(); ++i) {
            const google::protobuf::FieldDescriptor *field = fields[i];
            if (!field->is_repeated() && !reflection->HasField(message, field)) {
                // Field that has not been set
                if (field->is_required()) {
                    return turbo::data_loss_error("Missing required field: ", field->full_name());
                }
                // Whether dumps default fields
                if (!_option.always_print_primitive_fields) {
                    continue;
                }
            } else if (field->is_repeated()
                       && reflection->FieldSize(message, field) == 0
                       && !_option.jsonify_empty_array) {
                // Repeated field that has no entry
                continue;
            }

            const std::string &orig_name = field->name();
            bool decoded = decode_name(orig_name, field_name_str);
            const std::string &name = decoded ? field_name_str : orig_name;
            start_object(name);
            TURBO_RETURN_NOT_OK(pb_field_to_json(message, field, handler));
            end_object();
        }

        // Fill in map fields
        for (size_t i = 0; i < map_fields.size(); ++i) {
            const google::protobuf::FieldDescriptor *map_desc = map_fields[i];
            const google::protobuf::FieldDescriptor *key_desc =
                    map_desc->message_type()->field(merak::KEY_INDEX);
            const google::protobuf::FieldDescriptor *value_desc =
                    map_desc->message_type()->field(merak::VALUE_INDEX);

            // Write a json object corresponding to hold protobuf map
            // such as {"key": value, ...}
            const std::string &orig_name = map_desc->name();
            bool decoded = decode_name(orig_name, field_name_str);
            const std::string &name = decoded ? field_name_str : orig_name;
            start_object(name);
            std::string entry_name;
            for (int j = 0; j < reflection->FieldSize(message, map_desc); ++j) {
                const google::protobuf::Message &entry =
                        reflection->GetRepeatedMessage(message, map_desc, j);
                const google::protobuf::Reflection *entry_reflection = entry.GetReflection();
                entry_name = entry_reflection->GetStringReference(
                        entry, key_desc, &entry_name);
                start_object(entry_name);
                // Fill in entries into this json object
                TURBO_RETURN_NOT_OK(pb_field_to_json(entry, value_desc, handler));
                end_object();
            }
            end_object();
        }
        // Hack: Pass 0 as parameter since Writer doesn't care this
        if (root_msg) {
            handler.end_object(0);
        }

        return turbo::OkStatus();
    }

    template<typename Handler>
    turbo::Status PbToFlatConverter::pb_field_to_any(const google::protobuf::Message &message,
                                                   const google::protobuf::FieldDescriptor *field,
                                                   Handler &handler) {
        const google::protobuf::Reflection *reflection = message.GetReflection();
        if (field->is_repeated()) {
            int field_size = reflection->FieldSize(message, field);
            for (int index = 0; index < field_size; ++index) {
                start_object(turbo::str_format("[%d]",index));
                TURBO_RETURN_NOT_OK(pb_field_to_any_single(reflection->GetRepeatedMessage(
                        message, field, index), field, handler));
                end_object();
            }

        } else {
            TURBO_RETURN_NOT_OK(pb_field_to_any_single(reflection->GetMessage(message, field), field, handler));
        }
        return turbo::OkStatus();
    }

    template<typename Handler>
    turbo::Status PbToFlatConverter::pb_field_to_any_single(const google::protobuf::Message &message,
                         const google::protobuf::FieldDescriptor *field,
                         Handler &handler) {
        const google::protobuf::Reflection *reflection = message.GetReflection();
        std::string key_value;
        auto key_des = field->message_type()->field(0);
        key_value = reflection->GetStringReference(message, key_des, &key_value);
        if (_option.using_a_type_url) {
            auto k = key(A_TYPE_URL);
            handler.emplace_key(k.data(), k.size(), true);
        } else {
            auto k = key(TYPE_URL);
            handler.emplace_key(k.data(), k.size(), true);
        }
        handler.emplace_string(key_value.data(), key_value.size(), false);
        auto value_des = field->message_type()->field(1);
        std::string value_bytes;
        value_bytes = reflection->GetStringReference(message, value_des, &value_bytes);
        auto k = key(VALUE_NAME);
        handler.emplace_key(k.data(), k.size(), true);
        std::string b64;
        turbo::base64_encode(value_bytes, &b64);
        handler.emplace_string(b64.data(), b64.size(), false);
        return turbo::OkStatus();
    }

    template<typename Handler>
    turbo::Status PbToFlatConverter::pb_field_to_json(const google::protobuf::Message &message,
                                             const google::protobuf::FieldDescriptor *field, Handler &handler) {
        if(is_protobuf_any(field)) {
            return pb_field_to_any(message,field,handler);
        }
        const google::protobuf::Reflection *reflection = message.GetReflection();
        switch (field->cpp_type()) {
            case google::protobuf::FieldDescriptor::CPPTYPE_BOOL: {
                if (field->is_repeated()) {
                    int field_size = reflection->FieldSize(message, field);
                    for (int index = 0; index < field_size; ++index) {
                        auto k  = key(turbo::str_format("[%d]",index));
                        handler.emplace_key(k.data(), k.size(), false);
                        handler.emplace_bool(static_cast<bool>(reflection->GetRepeatedBool(message, field, index)));
                    }
                }
                else {
                    auto k  = key("");
                    handler.emplace_key(k.data(), k.size(), false);
                    handler.emplace_bool(static_cast<bool>(reflection->GetBool(message, field)));
                }
                break;
            }
            case google::protobuf::FieldDescriptor::CPPTYPE_INT32: {
                if (field->is_repeated()) {
                    int field_size = reflection->FieldSize(message, field);
                    for (int index = 0; index < field_size; ++index) {
                        auto k  = key(turbo::str_format("[%d]",index));
                        handler.emplace_key(k.data(), k.size(), false);
                        handler.emplace_int32(static_cast<int>(reflection->GetRepeatedInt32(message, field, index)));
                    }
                }
                else {
                    auto k  = key("");
                    handler.emplace_key(k.data(), k.size(), false);
                    handler.emplace_int32(static_cast<int>(reflection->GetInt32(message, field)));
                }
                break;
            }

            case google::protobuf::FieldDescriptor::CPPTYPE_UINT32: {
                if (field->is_repeated()) {
                    int field_size = reflection->FieldSize(message, field);
                    for (int index = 0; index < field_size; ++index) {
                        auto k  = key(turbo::str_format("[%d]",index));
                        handler.emplace_key(k.data(), k.size(), false);
                        handler.emplace_uint32(static_cast<unsigned int>(reflection->GetRepeatedUInt32(message, field, index)));
                    }
                }
                else {
                    auto k  = key("");
                    handler.emplace_key(k.data(), k.size(), false);
                    handler.emplace_uint32(static_cast<unsigned int>(reflection->GetUInt32(message, field)));
                }
                break;
            }
            case google::protobuf::FieldDescriptor::CPPTYPE_INT64: {
                if (field->is_repeated()) {
                    int field_size = reflection->FieldSize(message, field);
                    for (int index = 0; index < field_size; ++index) {
                        auto k  = key(turbo::str_format("[%d]",index));
                        handler.emplace_key(k.data(), k.size(), false);
                        handler.emplace_int64(static_cast<int64_t>(reflection->GetRepeatedInt64(message, field, index)));
                    }
                }
                else {
                    auto k  = key("");
                    handler.emplace_key(k.data(), k.size(), false);
                    handler.emplace_int64(static_cast<int64_t>(reflection->GetInt64(message, field)));
                }
                break;
            }
            case google::protobuf::FieldDescriptor::CPPTYPE_UINT64: {
                if (field->is_repeated()) {
                    int field_size = reflection->FieldSize(message, field);
                    for (int index = 0; index < field_size; ++index) {
                        auto k  = key(turbo::str_format("[%d]",index));
                        handler.emplace_key(k.data(), k.size(), false);
                        handler.emplace_uint64(static_cast<uint64_t>(reflection->GetRepeatedUInt64(message, field, index)));
                    }
                }
                else {
                    auto k  = key("");
                    handler.emplace_key(k.data(), k.size(), false);
                    handler.emplace_uint64(static_cast<uint64_t>(reflection->GetUInt64(message, field)));
                }
                break;
            }
            case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT: {
                if (field->is_repeated()) {
                    int field_size = reflection->FieldSize(message, field);
                    for (int index = 0; index < field_size; ++index) {
                        auto k  = key(turbo::str_format("[%d]",index));
                        handler.emplace_key(k.data(), k.size(), false);
                        handler.emplace_double(static_cast<double>(reflection->GetRepeatedFloat(message, field, index)));
                    }
                }
                else {
                    auto k  = key("");
                    handler.emplace_key(k.data(), k.size(), false);
                    handler.emplace_double(static_cast<double>(reflection->GetFloat(message, field)));
                }
                break;
            }
            case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE: {
                if (field->is_repeated()) {
                    int field_size = reflection->FieldSize(message, field);
                    for (int index = 0; index < field_size; ++index) {
                        auto k  = key(turbo::str_format("[%d]",index));
                        handler.emplace_key(k.data(), k.size(), false);
                        handler.emplace_double(static_cast<double>(reflection->GetRepeatedDouble(message, field, index)));
                    }
                }
                else {
                    auto k  = key("");
                    handler.emplace_key(k.data(), k.size(), false);
                    handler.emplace_double(static_cast<double>(reflection->GetDouble(message, field)));
                }
                break;
            }

            case google::protobuf::FieldDescriptor::CPPTYPE_STRING: {
                std::string value;
                if (field->is_repeated()) {
                    int field_size = reflection->FieldSize(message, field);
                    for (int index = 0; index < field_size; ++index) {
                        auto k  = key(turbo::str_format("[%d]",index));
                        handler.emplace_key(k.data(), k.size(), false);
                        value = reflection->GetRepeatedStringReference(
                                message, field, index, &value);
                        if (field->type() == google::protobuf::FieldDescriptor::TYPE_BYTES
                            && _option.bytes_to_base64) {
                            std::string value_decoded;
                            turbo::base64_encode(value, &value_decoded);
                            handler.emplace_string(value_decoded.data(), value_decoded.size(), false);
                        } else {
                            handler.emplace_string(value.data(), value.size(), false);
                        }
                    }

                } else {
                    value = reflection->GetStringReference(message, field, &value);
                    auto k  = key("");
                    handler.emplace_key(k.data(), k.size(), false);
                    if (field->type() == google::protobuf::FieldDescriptor::TYPE_BYTES
                        && _option.bytes_to_base64) {
                        std::string value_decoded;
                        turbo::base64_encode(value, &value_decoded);
                        handler.emplace_string(value_decoded.data(), value_decoded.size(), false);
                    } else {
                        handler.emplace_string(value.data(), value.size(), false);
                    }
                }
                break;
            }

            case google::protobuf::FieldDescriptor::CPPTYPE_ENUM: {
                if (field->is_repeated()) {
                    int field_size = reflection->FieldSize(message, field);
                    if (_option.enum_option == OUTPUT_ENUM_BY_NAME) {
                        for (int index = 0; index < field_size; ++index) {
                            auto k  = key(turbo::str_format("[%d]",index));
                            handler.emplace_key(k.data(), k.size(), false);
                            const std::string &enum_name = reflection->GetRepeatedEnum(
                                    message, field, index)->name();
                            handler.emplace_string(enum_name.data(), enum_name.size(), false);
                        }
                    } else {
                        for (int index = 0; index < field_size; ++index) {
                            auto k  = key(turbo::str_format("[%d]",index));
                            handler.emplace_key(k.data(), k.size(), false);
                            handler.emplace_int32(reflection->GetRepeatedEnum(
                                    message, field, index)->number());
                        }
                    }

                } else {
                    auto k  = key("");
                    handler.emplace_key(k.data(), k.size(), false);
                    if (_option.enum_option == OUTPUT_ENUM_BY_NAME) {
                        const std::string &enum_name =
                                reflection->GetEnum(message, field)->name();
                        handler.emplace_string(enum_name.data(), enum_name.size(), false);
                    } else {
                        handler.emplace_int32(reflection->GetEnum(message, field)->number());
                    }
                }
                break;
            }

            case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE: {
                if (field->is_repeated()) {
                    int field_size = reflection->FieldSize(message, field);
                    for (int index = 0; index < field_size; ++index) {
                        start_object(turbo::str_format("[%d]",index));
                        TURBO_RETURN_NOT_OK(Convert(reflection->GetRepeatedMessage(
                                message, field, index), handler));
                        end_object();
                    }

                } else {
                    TURBO_RETURN_NOT_OK(Convert(reflection->GetMessage(message, field), handler));
                }
                break;
            }
        }
        return turbo::OkStatus();
    }

    template<typename OutputStream>
    turbo::Status proto_message_to_json_stream(const google::protobuf::Message &message,
                                      const Pb2FlatOptions &options,
                                      OutputStream &os) {
        PbToFlatConverter converter(options);
        if (options.pretty_json) {
            merak::json::PrettyWriter<OutputStream> writer(os);
            return converter.Convert(message, writer, true);
        } else {
            merak::json::Writer<OutputStream> writer(os);
            return converter.Convert(message, writer, true);
        }
    }

    turbo::Status proto_message_to_json(const google::protobuf::Message &message,
                               std::string *json,
                               const Pb2FlatOptions &options) {
        merak::json::StringBuffer buffer;
        TURBO_RETURN_NOT_OK(merak::proto_message_to_json_stream(message, options, buffer));
        json->append(buffer.get_string(), buffer.GetSize());
        return turbo::OkStatus();
    }

    turbo::Status proto_message_to_flat_json(const google::protobuf::Message& message, std::string* json, const Pb2FlatOptions &op) {
        return proto_message_to_json(message, json, op);
    }

    turbo::Status proto_message_to_flat(const google::protobuf::Message &message,
                                        turbo::flat_hash_map<std::string, PrimitiveValue> &result,
                                        const Pb2FlatOptions &options) {
        PbToFlatConverter converter(options);
        FlatContainerHandler<turbo::flat_hash_map<std::string, PrimitiveValue>> writer(result);
        return converter.Convert(message, writer, true);
    }

    turbo::Status proto_message_to_flat(const google::protobuf::Message &message,
                                        turbo::flat_hash_map<std::string, std::string> &result,
                                        const Pb2FlatOptions &options) {
        PbToFlatConverter converter(options);
        FlatContainerHandler<turbo::flat_hash_map<std::string, std::string>> writer(result);
        return converter.Convert(message, writer, true);
    }

    turbo::Status proto_message_to_flat(const google::protobuf::Message &message,
                                        FlatHandlerBase &handler,
                                        const Pb2FlatOptions &options) {
        PbToFlatConverter converter(options);
        return converter.Convert(message, handler, true);
    }
} // namespace merak