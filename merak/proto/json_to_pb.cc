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
#include <merak/proto/json_to_pb.h>
#include <merak/utility/zero_copy_stream_reader.h>       // ZeroCopyStreamReader
#include <merak/proto/encode_decode.h>
#include <turbo/strings/str_format.h>
#include <merak/proto/descriptor.h>
#include <merak/json.h>
#include <turbo/strings/escaping.h>
#include <turbo/log/logging.h>

#define J2PERROR(perr, fmt, ...)                                    \
    J2PERROR_WITH_PB((::google::protobuf::Message*)nullptr, perr, fmt, ##__VA_ARGS__)

#define J2PERROR_WITH_PB(pb, perr, fmt, ...)                            \
    if (perr) {                                                         \
        if (!perr->empty()) {                                           \
            perr->append(", ", 2);                                      \
        }                                                               \
        turbo::str_append_format(perr, fmt, ##__VA_ARGS__);                \
        if ((pb) != nullptr) {                                            \
           turbo::str_append_format(perr, " [%s]", (pb)->GetDescriptor()->name().c_str());  \
        }                                                               \
    } else { }

namespace merak {

    enum MatchType {
        TYPE_MATCH = 0x00,
        REQUIRED_OR_REPEATED_TYPE_MISMATCH = 0x01,
        OPTIONAL_TYPE_MISMATCH = 0x02
    };

    static void string_append_value(const merak::json::Value &value,
                                    std::string *output) {
        if (value.is_null()) {
            output->append("null");
        } else if (value.is_bool()) {
            output->append(value.get_bool() ? "true" : "false");
        } else if (value.is_int32()) {
            turbo::str_append_format(output, "%d", value.get_int32());
        } else if (value.is_uint32()) {
            turbo::str_append_format(output, "%u", value.get_uint32());
        } else if (value.is_int64()) {
            turbo::str_append_format(output, "%" PRId64, value.get_int64());
        } else if (value.is_uint64()) {
            turbo::str_append_format(output, "%" PRIu64, value.get_uint64());
        } else if (value.is_double()) {
            turbo::str_append_format(output, "%f", value.get_double());
        } else if (value.is_string()) {
            output->push_back('"');
            output->append(value.get_string(), value.get_string_length());
            output->push_back('"');
        } else if (value.is_array()) {
            output->append("array");
        } else if (value.is_object()) {
            output->append("object");
        }
    }

    //It will be called when type mismatch occurs, fg: convert string to uint,
    //and will also be called when invalid value appears, fg: invalid enum name,
    //invalid enum number, invalid string content to convert to double or float.
    //for optional field error will just append error into error message
    //and ends with ',' and return true.
    //otherwise will append error into error message and return false.
    inline bool value_invalid(const google::protobuf::FieldDescriptor *field, const char *type,
                              const merak::json::Value &value, std::string *err) {
        bool optional = field->is_optional();
        if (err) {
            if (!err->empty()) {
                err->append(", ");
            }
            err->append("Invalid value `");
            string_append_value(value, err);
            turbo::str_append_format(err, "' for %sfield `%s' which SHOULD be %s",
                                     optional ? "optional " : "",
                                     field->full_name().c_str(), type);
        }
        if (!optional) {
            return false;
        }
        return true;
    }

    template<typename T>
    inline bool convert_string_to_double_float_type(
            void (google::protobuf::Reflection::*func)(
                    google::protobuf::Message *message,
                    const google::protobuf::FieldDescriptor *field, T value) const,
            google::protobuf::Message *message,
            const google::protobuf::FieldDescriptor *field,
            const google::protobuf::Reflection *reflection,
            const merak::json::Value &item,
            std::string *err) {
        const char *limit_type = item.get_string();  // MUST be string here
        if (std::numeric_limits<T>::has_quiet_NaN &&
            strcasecmp(limit_type, "NaN") == 0) {
            (reflection->*func)(message, field, std::numeric_limits<T>::quiet_NaN());
            return true;
        }
        if (std::numeric_limits<T>::has_infinity &&
            strcasecmp(limit_type, "Infinity") == 0) {
            (reflection->*func)(message, field, std::numeric_limits<T>::infinity());
            return true;
        }
        if (std::numeric_limits<T>::has_infinity &&
            strcasecmp(limit_type, "-Infinity") == 0) {
            (reflection->*func)(message, field, -std::numeric_limits<T>::infinity());
            return true;
        }
        return value_invalid(field, typeid(T).name(), item, err);
    }

    inline bool convert_float_type(const merak::json::Value &item, bool repeated,
                                   google::protobuf::Message *message,
                                   const google::protobuf::FieldDescriptor *field,
                                   const google::protobuf::Reflection *reflection,
                                   std::string *err) {
        if (item.is_number()) {
            if (repeated) {
                reflection->AddFloat(message, field, item.get_double());
            } else {
                reflection->SetFloat(message, field, item.get_double());
            }
        } else if (item.is_string()) {
            if (!convert_string_to_double_float_type(
                    (repeated ? &google::protobuf::Reflection::AddFloat
                              : &google::protobuf::Reflection::SetFloat),
                    message, field, reflection, item, err)) {
                return false;
            }
        } else {
            return value_invalid(field, "float", item, err);
        }
        return true;
    }

    inline bool convert_double_type(const merak::json::Value &item, bool repeated,
                                    google::protobuf::Message *message,
                                    const google::protobuf::FieldDescriptor *field,
                                    const google::protobuf::Reflection *reflection,
                                    std::string *err) {
        if (item.is_number()) {
            if (repeated) {
                reflection->AddDouble(message, field, item.get_double());
            } else {
                reflection->SetDouble(message, field, item.get_double());
            }
        } else if (item.is_string()) {
            if (!convert_string_to_double_float_type(
                    (repeated ? &google::protobuf::Reflection::AddDouble
                              : &google::protobuf::Reflection::SetDouble),
                    message, field, reflection, item, err)) {
                return false;
            }
        } else {
            return value_invalid(field, "double", item, err);
        }
        return true;
    }

    inline bool convert_enum_type(const merak::json::Value &item, bool repeated,
                                  google::protobuf::Message *message,
                                  const google::protobuf::FieldDescriptor *field,
                                  const google::protobuf::Reflection *reflection,
                                  std::string *err) {
        const google::protobuf::EnumValueDescriptor *enum_value_descriptor = nullptr;
        if (item.is_int32()) {
            enum_value_descriptor = field->enum_type()->FindValueByNumber(item.get_int32());
        } else if (item.is_string()) {
            enum_value_descriptor = field->enum_type()->FindValueByName(item.get_string());
        }
        if (!enum_value_descriptor) {
            return value_invalid(field, "enum", item, err);
        }
        if (repeated) {
            reflection->AddEnum(message, field, enum_value_descriptor);
        } else {
            reflection->SetEnum(message, field, enum_value_descriptor);
        }
        return true;
    }

    inline bool convert_int64_type(const merak::json::Value &item, bool repeated,
                                   google::protobuf::Message *message,
                                   const google::protobuf::FieldDescriptor *field,
                                   const google::protobuf::Reflection *reflection,
                                   std::string *err) {

        int64_t num;
        if (item.is_int64()) {
            if (repeated) {
                reflection->AddInt64(message, field, item.get_int64());
            } else {
                reflection->SetInt64(message, field, item.get_int64());
            }
        } else if (item.is_string() &&
                   turbo::simple_atoi({item.get_string(), item.get_string_length()},
                                      &num)) {
            if (repeated) {
                reflection->AddInt64(message, field, num);
            } else {
                reflection->SetInt64(message, field, num);
            }
        } else {
            return value_invalid(field, "INT64", item, err);
        }
        return true;
    }

    inline bool convert_uint64_type(const merak::json::Value &item,
                                    bool repeated,
                                    google::protobuf::Message *message,
                                    const google::protobuf::FieldDescriptor *field,
                                    const google::protobuf::Reflection *reflection,
                                    std::string *err) {
        uint64_t num;
        if (item.is_uint64()) {
            if (repeated) {
                reflection->AddUInt64(message, field, item.get_uint64());
            } else {
                reflection->SetUInt64(message, field, item.get_uint64());
            }
        } else if (item.is_string() &&
                   turbo::simple_atoi({item.get_string(), item.get_string_length()},
                                      &num)) {
            if (repeated) {
                reflection->AddUInt64(message, field, num);
            } else {
                reflection->SetUInt64(message, field, num);
            }
        } else {
            return value_invalid(field, "UINT64", item, err);
        }
        return true;
    }

    bool json_value_to_proto_message(const merak::json::Value &json_value,
                                 google::protobuf::Message *message,
                                 const Json2PbOptions &options,
                                 std::string *err,
                                 bool root_val = false);

//Json value to protobuf convert rules for type:
//Json value type                 Protobuf type                convert rules
//int                             int uint int64 uint64        valid convert is available
//uint                            int uint int64 uint64        valid convert is available
//int64                           int uint int64 uint64        valid convert is available
//uint64                          int uint int64 uint64        valid convert is available
//int uint int64 uint64           float double                 available
//"NaN" "Infinity" "-Infinity"    float double                 only "NaN" "Infinity" "-Infinity" is available
//int                             enum                         valid enum number value is available
//string                          enum                         valid enum name value is available
//string                          int64 uint64                 valid convert is available
//other mismatch type convertion will be regarded as error.
#define J2PCHECKTYPE(value, cpptype, jsontype) ({                   \
            MatchType match_type = TYPE_MATCH;                      \
            if (!value.is_##jsontype()) {                            \
                match_type = OPTIONAL_TYPE_MISMATCH;                \
                if (!value_invalid(field, #cpptype, value, err)) {  \
                    return false;                                   \
                }                                                   \
            }                                                       \
            match_type;                                             \
        })

    bool json_to_proto_any_one(const merak::json::Value &value,
                               const google::protobuf::FieldDescriptor *any_desc,
                               google::protobuf::Message *message,
                               const Json2PbOptions &options,
                               std::string *err) {
        if (!value.is_object()) {
            J2PERROR(err, "Non-object value for Any field: %s",
                     any_desc->full_name().c_str());
            return false;
        }
        static std::string kTypeUrl(TYPE_URL);
        static std::string kATypeUrl(A_TYPE_URL);
        static std::string kValue(VALUE_NAME);
        auto type_url_it = value.find_member(kTypeUrl);

        if(type_url_it == value.member_end()) {
            type_url_it = value.find_member(kATypeUrl);
        }
        if(type_url_it == value.member_end() || !type_url_it->value.is_string()) {
            std::cout<<"no type_url_it"<<std::endl;
            return false;
        }
        std::string type_url = type_url_it->value.get_string();
        std::string_view type_url_view = type_url;
        if(turbo::starts_with(type_url_view, TYPE_PREFIX)) {
            type_url_view = type_url_view.substr(TYPE_PREFIX.size());
        }
        std::unique_ptr<google::protobuf::Message> in_msg(create_message_by_type_name(type_url_view));
        if(!in_msg) {
            return false;
        }
        auto value_it = value.find_member(kValue);
        if(value_it == value.member_end() || !value_it->value.is_object()) {
            return false;
        }

        if(!json_value_to_proto_message(value_it->value, in_msg.get(), options, err, true)) {
            return false;
        }

        auto value_data = in_msg->SerializeAsString();

        const google::protobuf::FieldDescriptor *key_desc =
                any_desc->message_type()->FindFieldByName(absl::string_view(TYPE_URL.data(),TYPE_URL.size()));
        const google::protobuf::FieldDescriptor *value_desc =
                any_desc->message_type()->FindFieldByName(absl::string_view(VALUE_NAME.data(),VALUE_NAME.size()));
        const google::protobuf::Reflection *entry_reflection = message->GetReflection();
        entry_reflection->SetString(message, key_desc, turbo::str_cat(TYPE_PREFIX, type_url));
        entry_reflection->SetString(message, value_desc, value_data);
        return true;
    }

    bool json_to_proto_any(const merak::json::Value &value,
                           const google::protobuf::FieldDescriptor *field,
                           google::protobuf::Message *message,
                           const Json2PbOptions &options,
                           std::string *err) {
        const google::protobuf::Reflection *reflection = message->GetReflection();
        if (field->is_repeated()) {
            const merak::json::SizeType size = value.size();
            for (merak::json::SizeType index = 0; index < size; ++index) {
                const merak::json::Value &item = value[index];
                if (TYPE_MATCH == J2PCHECKTYPE(item, message, object)) {
                    if (!json_to_proto_any_one(
                            item, field, reflection->AddMessage(message, field), options, err)) {
                        return false;
                    }
                }
            }
        } else if (!json_to_proto_any_one(
                value, field, reflection->MutableMessage(message, field), options, err)) {
            return false;
        }

        return true;
    }

    static bool json_value_to_proto_field(const merak::json::Value &value,
                                      const google::protobuf::FieldDescriptor *field,
                                      google::protobuf::Message *message,
                                      const Json2PbOptions &options,
                                      std::string *err) {
        if(is_protobuf_any(field)) {
            return json_to_proto_any(value,field,message,options,err);
        }
        if (value.is_null()) {
            if (field->is_required()) {
                J2PERROR(err, "Missing required field: %s", field->full_name().c_str());
                return false;
            }
            return true;
        }

        if (field->is_repeated()) {
            if (!value.is_array()) {
                J2PERROR(err, "Invalid value for repeated field: %s",
                         field->full_name().c_str());
                return false;
            }
        }

        const google::protobuf::Reflection *reflection = message->GetReflection();
        switch (field->cpp_type()) {
#define CASE_FIELD_TYPE(cpptype, method, jsontype)                      \
        case google::protobuf::FieldDescriptor::CPPTYPE_##cpptype: {                      \
            if (field->is_repeated()) {                                 \
                const merak::json::SizeType size = value.size();          \
                for (merak::json::SizeType index = 0; index < size; ++index) { \
                    const merak::json::Value & item = value[index];       \
                    if (TYPE_MATCH == J2PCHECKTYPE(item, cpptype, jsontype)) { \
                        reflection->Add##method(message, field, item.get_##jsontype()); \
                    }                                                   \
                }                                                       \
            } else if (TYPE_MATCH == J2PCHECKTYPE(value, cpptype, jsontype)) { \
                reflection->Set##method(message, field, value.get_##jsontype()); \
            }                                                           \
            break;                                                      \
        }                                                               \

            CASE_FIELD_TYPE(INT32, Int32, int32);
            CASE_FIELD_TYPE(UINT32, UInt32, uint32);
            CASE_FIELD_TYPE(BOOL, Bool, bool);
#undef CASE_FIELD_TYPE

            case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
                if (field->is_repeated()) {
                    const merak::json::SizeType size = value.size();
                    for (merak::json::SizeType index = 0; index < size;
                         ++index) {
                        const merak::json::Value &item = value[index];
                        if (!convert_int64_type(item, true, message, field, reflection,
                                                err)) {
                            return false;
                        }
                    }
                } else if (!convert_int64_type(value, false, message, field, reflection,
                                               err)) {
                    return false;
                }
                break;

            case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
                if (field->is_repeated()) {
                    const merak::json::SizeType size = value.size();
                    for (merak::json::SizeType index = 0; index < size;
                         ++index) {
                        const merak::json::Value &item = value[index];
                        if (!convert_uint64_type(item, true, message, field, reflection,
                                                 err)) {
                            return false;
                        }
                    }
                } else if (!convert_uint64_type(value, false, message, field, reflection,
                                                err)) {
                    return false;
                }
                break;

            case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
                if (field->is_repeated()) {
                    const merak::json::SizeType size = value.size();
                    for (merak::json::SizeType index = 0; index < size; ++index) {
                        const merak::json::Value &item = value[index];
                        if (!convert_float_type(item, true, message, field,
                                                reflection, err)) {
                            return false;
                        }
                    }
                } else if (!convert_float_type(value, false, message, field,
                                               reflection, err)) {
                    return false;
                }
                break;

            case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
                if (field->is_repeated()) {
                    const merak::json::SizeType size = value.size();
                    for (merak::json::SizeType index = 0; index < size; ++index) {
                        const merak::json::Value &item = value[index];
                        if (!convert_double_type(item, true, message, field,
                                                 reflection, err)) {
                            return false;
                        }
                    }
                } else if (!convert_double_type(value, false, message, field,
                                                reflection, err)) {
                    return false;
                }
                break;

            case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
                if (field->is_repeated()) {
                    const merak::json::SizeType size = value.size();
                    for (merak::json::SizeType index = 0; index < size; ++index) {
                        const merak::json::Value &item = value[index];
                        if (TYPE_MATCH == J2PCHECKTYPE(item, string, string)) {
                            std::string str(item.get_string(), item.get_string_length());
                            if (field->type() == google::protobuf::FieldDescriptor::TYPE_BYTES &&
                                options.base64_to_bytes) {
                                std::string str_decoded;
                                if (!turbo::base64_decode(str, &str_decoded)) {
                                    J2PERROR_WITH_PB(message, err, "Fail to decode base64 string=%s", str.c_str());
                                    return false;
                                }
                                str = str_decoded;
                            }
                            reflection->AddString(message, field, str);
                        }
                    }
                } else if (TYPE_MATCH == J2PCHECKTYPE(value, string, string)) {
                    std::string str(value.get_string(), value.get_string_length());
                    if (field->type() == google::protobuf::FieldDescriptor::TYPE_BYTES &&
                        options.base64_to_bytes) {
                        std::string str_decoded;
                        if (!turbo::base64_decode(str, &str_decoded)) {
                            J2PERROR_WITH_PB(message, err, "Fail to decode base64 string=%s", str.c_str());
                            return false;
                        }
                        str = str_decoded;
                    }
                    reflection->SetString(message, field, str);
                }
                break;

            case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
                if (field->is_repeated()) {
                    const merak::json::SizeType size = value.size();
                    for (merak::json::SizeType index = 0; index < size; ++index) {
                        const merak::json::Value &item = value[index];
                        if (!convert_enum_type(item, true, message, field,
                                               reflection, err)) {
                            return false;
                        }
                    }
                } else if (!convert_enum_type(value, false, message, field,
                                              reflection, err)) {
                    return false;
                }
                break;

            case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
                if (field->is_repeated()) {
                    const merak::json::SizeType size = value.size();
                    for (merak::json::SizeType index = 0; index < size; ++index) {
                        const merak::json::Value &item = value[index];
                        if (TYPE_MATCH == J2PCHECKTYPE(item, message, object)) {
                            if (!json_value_to_proto_message(
                                    item, reflection->AddMessage(message, field), options, err)) {
                                return false;
                            }
                        }
                    }
                } else if (!json_value_to_proto_message(
                        value, reflection->MutableMessage(message, field), options, err)) {
                    return false;
                }
                break;
        }
        return true;
    }


    bool json_map_to_proto_map(const merak::json::Value &value,
                           const google::protobuf::FieldDescriptor *map_desc,
                           google::protobuf::Message *message,
                           const Json2PbOptions &options,
                           std::string *err) {
        if (!value.is_object()) {
            J2PERROR(err, "Non-object value for map field: %s",
                     map_desc->full_name().c_str());
            return false;
        }

        const google::protobuf::Reflection *reflection = message->GetReflection();
        const google::protobuf::FieldDescriptor *key_desc =
                map_desc->message_type()->FindFieldByName(absl::string_view(KEY_NAME.data(),KEY_NAME.size()));
        const google::protobuf::FieldDescriptor *value_desc =
                map_desc->message_type()->FindFieldByName(absl::string_view(VALUE_NAME.data(),VALUE_NAME.size()));

        for (merak::json::Value::ConstMemberIterator it =
                value.member_begin(); it != value.member_end(); ++it) {
            google::protobuf::Message *entry = reflection->AddMessage(message, map_desc);
            const google::protobuf::Reflection *entry_reflection = entry->GetReflection();
            entry_reflection->SetString(
                    entry, key_desc, std::string(it->name.get_string(),
                                                 it->name.get_string_length()));
            if (!json_value_to_proto_field(it->value, value_desc, entry, options, err)) {
                return false;
            }
        }
        return true;
    }

    bool json_value_to_proto_message(const merak::json::Value &json_value,
                                 google::protobuf::Message *message,
                                 const Json2PbOptions &options,
                                 std::string *err,
                                 bool root_val) {
        const google::protobuf::Descriptor *descriptor = message->GetDescriptor();
        if (!json_value.is_object() &&
            !(json_value.is_array() && options.array_to_single_repeated && root_val)) {
            J2PERROR_WITH_PB(message, err, "The input is not a json object");
            return false;
        }

        const google::protobuf::Reflection *reflection = message->GetReflection();

        std::vector<const google::protobuf::FieldDescriptor *> fields;
        fields.reserve(64);
        for (int i = 0; i < descriptor->extension_range_count(); ++i) {
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
        for (int i = 0; i < descriptor->field_count(); ++i) {
            fields.push_back(descriptor->field(i));
        }

        if (json_value.is_array()) {
            if (fields.size() == 1 && fields.front()->is_repeated()) {
                return json_value_to_proto_field(json_value, fields.front(), message, options, err);
            }

            J2PERROR_WITH_PB(message, err, "the input json can't be array here");
            return false;
        }

        std::string field_name_str_temp;
        const merak::json::Value *value_ptr = nullptr;
        for (size_t i = 0; i < fields.size(); ++i) {
            const google::protobuf::FieldDescriptor *field = fields[i];

            const std::string &orig_name = field->name();
            bool res = decode_name(orig_name, field_name_str_temp);
            const std::string &field_name_str = (res ? field_name_str_temp : orig_name);
            merak::json::Value::ConstMemberIterator member =
                    json_value.find_member(field_name_str.c_str());
            if(member == json_value.member_end() && options.compatible_json_field) {
               auto [trans, changed] =  change_camel_style(field_name_str);
               if(changed) {
                   member = json_value.find_member(trans.c_str());
               }
            }
            if (member == json_value.member_end()) {
                if (field->is_required()) {
                    J2PERROR(err, "Missing required field: %s", field->full_name().c_str());
                    return false;
                }
                continue;
            }
            value_ptr = &(member->value);
            if (is_protobuf_map(field) && value_ptr->is_object()) {
                // Try to parse json like {"key":value, ...} into protobuf map
                if (!json_map_to_proto_map(*value_ptr, field, message, options, err)) {
                    return false;
                }
            } else {
                if (!json_value_to_proto_field(*value_ptr, field, message, options, err)) {
                    return false;
                }
            }
        }
        return true;
    }

    inline turbo::Status json_to_proto_message_inline(const std::string &json_string,
                                                      turbo::Nonnull<google::protobuf::Message*> message,
                                         const Json2PbOptions &options,
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
                                                 merak::json::get_parse_error_en(d.get_parse_error()), " [", message->GetDescriptor()->name(), "]");
        }
        std::string err;
        auto r = json_value_to_proto_message(d, message, options, &err, true);
        if(TURBO_UNLIKELY(!r)) {
            return turbo::invalid_argument_error(err);
        }
        /*
        if(TURBO_UNLIKELY(!err.empty())) {
            std::cout<<err<<std::endl;
            return turbo::OkStatus().set_kv_payload("optional",err);
        }*/
        return turbo::OkStatus();
    }

    turbo::Status json_to_proto_message(const merak::json::Value &json,
                               google::protobuf::Message *message,
                               const Json2PbOptions &options) {
        std::string err;
        auto r = json_value_to_proto_message(json, message, options, &err, true);
        if(TURBO_UNLIKELY(!r)) {
            return turbo::invalid_argument_error(err);
        }
        /*
        if(TURBO_UNLIKELY(!err.empty())) {
            std::cout<<err<<std::endl;
            return turbo::OkStatus().set_kv_payload("optional",err);
        }*/
        return turbo::OkStatus();
    }

    turbo::Status json_to_proto_message(const std::string &json_string,
                                        turbo::Nonnull<google::protobuf::Message*> message,
                            const Json2PbOptions &options,
                            size_t *parsed_offset) {
        return json_to_proto_message_inline(json_string, message, options, parsed_offset);
    }

    turbo::Status json_to_proto_message(google::protobuf::io::ZeroCopyInputStream *stream,
                                        turbo::Nonnull<google::protobuf::Message*> message,
                            const Json2PbOptions &options,
                            size_t *parsed_offset) {
        ZeroCopyStreamReader reader(stream);
        return json_to_proto_message(&reader, message, options, parsed_offset);
    }

    turbo::Status json_to_proto_message(ZeroCopyStreamReader *reader,
                                        turbo::Nonnull<google::protobuf::Message*> message,
                            const Json2PbOptions &options,
                            size_t *parsed_offset) {
        merak::json::Document d;
        if (options.allow_remaining_bytes_after_parsing) {
            d.parse_stream<merak::json::kParseStopWhenDoneFlag, merak::json::UTF8<>>(
                    *reader);
            if (parsed_offset != nullptr) {
                *parsed_offset = d.get_error_offset();
            }
        } else {
            d.parse_stream<0, merak::json::UTF8<>>(*reader);
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
                                                 merak::json::get_parse_error_en(d.get_parse_error()), " [", message->GetDescriptor()->name(), "]");
        }
        std::string err;
        auto r = json_value_to_proto_message(d, message, options, &err, true);
        if(TURBO_UNLIKELY(!r)) {
            return turbo::invalid_argument_error(err);
        }
        /*
        if(TURBO_UNLIKELY(!err.empty())) {
            std::cout<<err<<std::endl;
            return turbo::OkStatus().set_kv_payload("optional",err);
        }*/
        return turbo::OkStatus();
    }

    turbo::Status json_to_proto_message(const std::string &json_string,
                                        turbo::Nonnull<google::protobuf::Message*> message) {
        return json_to_proto_message_inline(json_string, message, Json2PbOptions(), nullptr);
    }


    turbo::Status json_to_proto_message(google::protobuf::io::ZeroCopyInputStream *stream,
                                        turbo::Nonnull<google::protobuf::Message*> message) {
        return json_to_proto_message(stream, message, Json2PbOptions(),nullptr);
    }

} //namespace merak

#undef J2PERROR
#undef J2PCHECKTYPE
