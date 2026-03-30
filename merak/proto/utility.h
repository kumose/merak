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
#include <merak/utility/zero_copy_stream_reader.h>       // ZeroCopyStreamReader
#include <merak/proto/encode_decode.h>
#include <turbo/strings/str_format.h>
#include <merak/proto/descriptor.h>
#include <merak/json.h>
#include <turbo/strings/escaping.h>
#include <turbo/log/logging.h>
#include <merak/utility/zero_copy_stream_reader.h>
#include <merak/json.h>
#include <turbo/utility/status.h>
#include <google/protobuf/message.h>
#include <google/protobuf/io/zero_copy_stream.h>


namespace merak {

    template<typename... Args>
    void json_to_pb_error(google::protobuf::Message *msg, std::string * dst,
                                 const turbo::FormatSpec<Args...> &format,
                                 const Args &... args) {
        if (dst) {
            if (!dst->empty()) {
                dst->append(", ", 2);
            }
            turbo::str_append_format(dst, format, args...);
            if ((msg) != nullptr) {
                turbo::str_append_format(dst, " [%s]", (msg)->GetDescriptor()->name().c_str());
            }
        }

    }

    template<typename... Args>
    void json_to_pb_error(std::string * dst,
                                 const turbo::FormatSpec<Args...> &format,
                                 const Args &... args) {
        if (dst) {
            if (!dst->empty()) {
                dst->append(", ", 2);
            }
            turbo::str_append_format(dst, format, args...);
        }

    }

    enum MatchType {
        TYPE_MATCH = 0x00,
        REQUIRED_OR_REPEATED_TYPE_MISMATCH = 0x01,
        OPTIONAL_TYPE_MISMATCH = 0x02
    };

    inline void string_append_value(const merak::json::Value &value,
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



}