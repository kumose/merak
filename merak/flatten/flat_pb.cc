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

#include <merak/flatten/flat_pb.h>

#include <cstdlib>
#include <cstring>
#include <climits>
#include <memory>
#include <string>
#include <vector>

#include <google/protobuf/any.pb.h>
#include <google/protobuf/descriptor.h>
#include <turbo/strings/match.h>
#include <turbo/strings/numbers.h>
#include <turbo/strings/str_format.h>

#include <merak/proto/descriptor.h>

namespace merak {
namespace {

using google::protobuf::Descriptor;
using google::protobuf::FieldDescriptor;
using google::protobuf::Message;
using google::protobuf::Reflection;

struct LeafTarget {
    Message *msg{nullptr};
    const FieldDescriptor *fd{nullptr};
    int rep_index{-1};
};

struct AnyUnwindLayer {
    Message *any_submsg{nullptr};
    std::unique_ptr<Message> payload;
};

static bool is_any_well_known(const Descriptor *d) {
    return d != nullptr && d->full_name() == "google.protobuf.Any";
}

static bool is_direct_any_segment(std::string_view seg) {
    return seg == TYPE_URL || seg == A_TYPE_URL || seg == VALUE_NAME;
}

static turbo::Status unwrap_any_for_inner_path(
        Message *any_msg,
        std::vector<AnyUnwindLayer> *any_unwind,
        Message **out_cur,
        const Reflection **out_refl) {
    if (!any_unwind) {
        return turbo::invalid_argument_error("Any inner path requires unwind buffer");
    }
    const Reflection *ar = any_msg->GetReflection();
    const Descriptor *ad = any_msg->GetDescriptor();
    const FieldDescriptor *type_fd = ad->FindFieldByName(std::string(TYPE_URL));
    const FieldDescriptor *value_fd = ad->FindFieldByName(std::string(VALUE_NAME));
    if (!type_fd || !value_fd) {
        return turbo::invalid_argument_error("google.protobuf.Any missing type_url or value");
    }
    const std::string type_url_storage = ar->GetString(*any_msg, type_fd);
    std::string_view tv(type_url_storage);
    if (turbo::starts_with(tv, TYPE_PREFIX)) {
        tv.remove_prefix(TYPE_PREFIX.size());
    }
    if (tv.empty()) {
        return turbo::invalid_argument_error(
                "Any type_url is empty; set type_url (or @type) before inner fields");
    }
    std::unique_ptr<Message> inner(create_message_by_type_name(tv));
    if (!inner) {
        return turbo::not_found_error(
                turbo::str_format("unknown Any type_url message: %s", std::string(tv).c_str()));
    }
    const std::string payload = ar->GetString(*any_msg, value_fd);
    if (!inner->ParseFromArray(payload.data(), static_cast<int>(payload.size()))) {
        return turbo::invalid_argument_error("invalid Any serialized value bytes");
    }
    any_unwind->push_back(AnyUnwindLayer{any_msg, std::move(inner)});
    Message *loaded = any_unwind->back().payload.get();
    *out_cur = loaded;
    *out_refl = loaded->GetReflection();
    return turbo::OkStatus();
}

static turbo::Status repack_any_layers(std::vector<AnyUnwindLayer> &layers) {
    for (auto it = layers.rbegin(); it != layers.rend(); ++it) {
        google::protobuf::Any packed;
        if (!packed.PackFrom(*it->payload)) {
            return turbo::invalid_argument_error("Any PackFrom failed");
        }
        it->any_submsg->CopyFrom(packed);
    }
    return turbo::OkStatus();
}

static google::protobuf::Message *find_or_create_map_entry(
        Message &parent,
        const Reflection *refl,
        const FieldDescriptor *map_field,
        const std::string &map_key) {
    const FieldDescriptor *key_fd = map_field->message_type()->field(KEY_INDEX);
    for (int j = 0; j < refl->FieldSize(parent, map_field); ++j) {
        Message *entry = refl->MutableRepeatedMessage(&parent, map_field, j);
        const Reflection *er = entry->GetReflection();
        if (er->GetString(*entry, key_fd) == map_key) {
            return entry;
        }
    }
    Message *entry = refl->AddMessage(&parent, map_field);
    entry->GetReflection()->SetString(entry, key_fd, map_key);
    return entry;
}

static turbo::Status ensure_repeated_size(Message *msg,
                                          const Reflection *refl,
                                          const FieldDescriptor *fd,
                                          int need) {
    while (refl->FieldSize(*msg, fd) < need) {
        switch (fd->cpp_type()) {
            case FieldDescriptor::CPPTYPE_INT32:
                refl->AddInt32(msg, fd, 0);
                break;
            case FieldDescriptor::CPPTYPE_INT64:
                refl->AddInt64(msg, fd, 0);
                break;
            case FieldDescriptor::CPPTYPE_UINT32:
                refl->AddUInt32(msg, fd, 0);
                break;
            case FieldDescriptor::CPPTYPE_UINT64:
                refl->AddUInt64(msg, fd, 0);
                break;
            case FieldDescriptor::CPPTYPE_DOUBLE:
                refl->AddDouble(msg, fd, 0.0);
                break;
            case FieldDescriptor::CPPTYPE_FLOAT:
                refl->AddFloat(msg, fd, 0.0f);
                break;
            case FieldDescriptor::CPPTYPE_BOOL:
                refl->AddBool(msg, fd, false);
                break;
            case FieldDescriptor::CPPTYPE_ENUM:
                refl->AddEnum(msg, fd, fd->enum_type()->value(0));
                break;
            case FieldDescriptor::CPPTYPE_STRING:
                refl->AddString(msg, fd, "");
                break;
            case FieldDescriptor::CPPTYPE_MESSAGE:
                refl->AddMessage(msg, fd);
                break;
            default:
                return turbo::invalid_argument_error("unsupported repeated field type");
        }
    }
    return turbo::OkStatus();
}

static turbo::Status resolve_leaf(Message &root,
                                  const std::vector<FlatKey> &keys,
                                  LeafTarget *out,
                                  std::vector<AnyUnwindLayer> *any_unwind) {
    Message *cur = &root;
    const Reflection *refl = cur->GetReflection();
    for (size_t i = 0; i < keys.size();) {
        if (keys[i].is_index) {
            return turbo::invalid_argument_error("invalid flat key segment order");
        }
        if (is_any_well_known(cur->GetDescriptor())) {
            if (!is_direct_any_segment(keys[i].key)) {
                TURBO_RETURN_NOT_OK(
                        unwrap_any_for_inner_path(cur, any_unwind, &cur, &refl));
                continue;
            }
        }
        std::string field_lookup = keys[i].key;
        if (is_any_well_known(cur->GetDescriptor()) && field_lookup == A_TYPE_URL) {
            field_lookup = std::string(TYPE_URL);
        }
        const FieldDescriptor *fd = cur->GetDescriptor()->FindFieldByName(field_lookup);
        if (!fd) {
            return turbo::not_found_error(
                    turbo::str_format("unknown field: %s", keys[i].key.c_str()));
        }

        if (is_protobuf_map(fd)) {
            if (i + 1 >= keys.size() || keys[i + 1].is_index) {
                return turbo::invalid_argument_error("map field requires a key segment");
            }
            Message *entry = find_or_create_map_entry(*cur, refl, fd, keys[i + 1].key);
            const FieldDescriptor *val_fd = fd->message_type()->field(VALUE_INDEX);
            i += 2;
            if (i >= keys.size()) {
                out->msg = entry;
                out->fd = val_fd;
                out->rep_index = -1;
                return turbo::OkStatus();
            }
            if (val_fd->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE) {
                return turbo::invalid_argument_error("extra path after map key for scalar map value");
            }
            cur = entry->GetReflection()->MutableMessage(entry, val_fd);
            refl = cur->GetReflection();
            continue;
        }

        const bool next_is_index =
                i + 1 < keys.size() && keys[i + 1].is_index;
        if (next_is_index) {
            if (!fd->is_repeated()) {
                return turbo::invalid_argument_error("index on non-repeated field");
            }
            const int idx = keys[i + 1].index;
            if (idx < 0) {
                return turbo::invalid_argument_error("negative array index");
            }
            if (i + 2 < keys.size()) {
                if (fd->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE) {
                    return turbo::invalid_argument_error(
                            "nested path through non-message repeated");
                }
                TURBO_RETURN_NOT_OK(ensure_repeated_size(cur, refl, fd, idx + 1));
                cur = refl->MutableRepeatedMessage(cur, fd, idx);
                refl = cur->GetReflection();
                i += 2;
                continue;
            }
            TURBO_RETURN_NOT_OK(ensure_repeated_size(cur, refl, fd, idx + 1));
            out->msg = cur;
            out->fd = fd;
            out->rep_index = idx;
            return turbo::OkStatus();
        }

        if (i + 1 == keys.size()) {
            if (fd->is_repeated()) {
                return turbo::invalid_argument_error("repeated field requires index in flat key");
            }
            out->msg = cur;
            out->fd = fd;
            out->rep_index = -1;
            return turbo::OkStatus();
        }

        if (fd->is_repeated()) {
            return turbo::invalid_argument_error("repeated field requires index in flat key");
        }
        if (fd->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE) {
            return turbo::invalid_argument_error(
                    "non-leaf flat segment must be a message field");
        }
        cur = refl->MutableMessage(cur, fd);
        refl = cur->GetReflection();
        ++i;
    }
    return turbo::invalid_argument_error("empty flat key");
}

static bool parse_bool_string(std::string_view s, bool *out) {
    if (s.size() == 4 && strncasecmp(s.data(), "true", 4) == 0) {
        *out = true;
        return true;
    }
    if (s.size() == 5 && strncasecmp(s.data(), "false", 5) == 0) {
        *out = false;
        return true;
    }
    if (s.size() == 1 && s[0] == '1') {
        *out = true;
        return true;
    }
    if (s.size() == 1 && s[0] == '0') {
        *out = false;
        return true;
    }
    return false;
}

static bool parse_double_string(std::string_view s, double *out) {
    std::string tmp(s);
    char *end = nullptr;
    *out = std::strtod(tmp.c_str(), &end);
    return end == tmp.c_str() + tmp.size() && end != tmp.c_str();
}

static turbo::Status set_leaf_primitive(LeafTarget &leaf, const PrimitiveValue & value) {
    Message *msg = leaf.msg;
    const FieldDescriptor *fd = leaf.fd;
    const Reflection *refl = msg->GetReflection();
    const bool rep = leaf.rep_index >= 0;
    const int ix = leaf.rep_index;

    if (value.is_nil()) {
        if (rep) {
            return turbo::invalid_argument_error("cannot clear repeated element with nil");
        }
        refl->ClearField(msg, fd);
        return turbo::OkStatus();
    }

    switch (fd->cpp_type()) {
        case FieldDescriptor::CPPTYPE_BOOL: {
            if (!value.is_boolean()) {
                return turbo::invalid_argument_error("type mismatch: field is bool");
            }
            const bool v = value.get_bool_confident();
            if (rep) {
                refl->SetRepeatedBool(msg, fd, ix, v);
            } else {
                refl->SetBool(msg, fd, v);
            }
            return turbo::OkStatus();
        }
        case FieldDescriptor::CPPTYPE_INT32: {
            int32_t v = 0;
            if (value.is_int32()) {
                v = value.get_int32_confident();
            } else if (value.is_int64()) {
                const int64_t t = value.get_int64_confident();
                if (t > INT32_MAX || t < INT32_MIN) {
                    return turbo::invalid_argument_error("int32 overflow");
                }
                v = static_cast<int32_t>(t);
            } else {
                return turbo::invalid_argument_error("type mismatch: field is int32");
            }
            if (rep) {
                refl->SetRepeatedInt32(msg, fd, ix, v);
            } else {
                refl->SetInt32(msg, fd, v);
            }
            return turbo::OkStatus();
        }
        case FieldDescriptor::CPPTYPE_UINT32: {
            uint32_t v = 0;
            if (value.is_uint32()) {
                v = value.get_uint32_confident();
            } else if (value.is_uint64()) {
                const uint64_t t = value.get_uint64_confident();
                if (t > UINT32_MAX) {
                    return turbo::invalid_argument_error("uint32 overflow");
                }
                v = static_cast<uint32_t>(t);
            } else {
                return turbo::invalid_argument_error("type mismatch: field is uint32");
            }
            if (rep) {
                refl->SetRepeatedUInt32(msg, fd, ix, v);
            } else {
                refl->SetUInt32(msg, fd, v);
            }
            return turbo::OkStatus();
        }
        case FieldDescriptor::CPPTYPE_INT64: {
            int64_t v = 0;
            if (!value.is_int64()) {
                return turbo::invalid_argument_error("type mismatch: field is int64");
            }
            v = value.get_int64_confident();
            if (rep) {
                refl->SetRepeatedInt64(msg, fd, ix, v);
            } else {
                refl->SetInt64(msg, fd, v);
            }
            return turbo::OkStatus();
        }
        case FieldDescriptor::CPPTYPE_UINT64: {
            uint64_t v = 0;
            if (!value.is_uint64()) {
                return turbo::invalid_argument_error("type mismatch: field is uint64");
            }
            v = value.get_uint64_confident();
            if (rep) {
                refl->SetRepeatedUInt64(msg, fd, ix, v);
            } else {
                refl->SetUInt64(msg, fd, v);
            }
            return turbo::OkStatus();
        }
        case FieldDescriptor::CPPTYPE_FLOAT: {
            float v = 0;
            if (value.is_float()) {
                v = value.get_float_confident();
            } else if (value.is_double()) {
                v = static_cast<float>(value.get_double_confident());
            } else {
                return turbo::invalid_argument_error("type mismatch: field is float");
            }
            if (rep) {
                refl->SetRepeatedFloat(msg, fd, ix, v);
            } else {
                refl->SetFloat(msg, fd, v);
            }
            return turbo::OkStatus();
        }
        case FieldDescriptor::CPPTYPE_DOUBLE: {
            double v = 0;
            if (value.is_double()) {
                v = value.get_double_confident();
            } else if (value.is_float()) {
                v = static_cast<double>(value.get_float_confident());
            } else {
                return turbo::invalid_argument_error("type mismatch: field is double");
            }
            if (rep) {
                refl->SetRepeatedDouble(msg, fd, ix, v);
            } else {
                refl->SetDouble(msg, fd, v);
            }
            return turbo::OkStatus();
        }
        case FieldDescriptor::CPPTYPE_STRING: {
            if (!value.is_string()) {
                return turbo::invalid_argument_error("type mismatch: field is string");
            }
            std::string v = value.get_string_confident();
            if (rep) {
                refl->SetRepeatedString(msg, fd, ix, v);
            } else {
                refl->SetString(msg, fd, v);
            }
            return turbo::OkStatus();
        }
        case FieldDescriptor::CPPTYPE_ENUM: {
            if (!value.is_int32()) {
                return turbo::invalid_argument_error("type mismatch: field is enum (need int32)");
            }
            const auto *ed =
                    fd->enum_type()->FindValueByNumber(value.get_int32_confident());
            if (!ed) {
                return turbo::invalid_argument_error("invalid enum value number");
            }
            if (rep) {
                refl->SetRepeatedEnum(msg, fd, ix, ed);
            } else {
                refl->SetEnum(msg, fd, ed);
            }
            return turbo::OkStatus();
        }
        case FieldDescriptor::CPPTYPE_MESSAGE:
            return turbo::invalid_argument_error("message field cannot be set from primitive");
        default:
            return turbo::invalid_argument_error("unsupported protobuf field type");
    }
}

static turbo::Status set_leaf_string(LeafTarget &leaf, std::string_view value) {
    Message *msg = leaf.msg;
    const FieldDescriptor *fd = leaf.fd;
    const Reflection *refl = msg->GetReflection();
    const bool rep = leaf.rep_index >= 0;
    const int ix = leaf.rep_index;

    switch (fd->cpp_type()) {
        case FieldDescriptor::CPPTYPE_BOOL: {
            bool v = false;
            if (!parse_bool_string(value, &v)) {
                return turbo::invalid_argument_error("invalid bool string");
            }
            if (rep) {
                refl->SetRepeatedBool(msg, fd, ix, v);
            } else {
                refl->SetBool(msg, fd, v);
            }
            return turbo::OkStatus();
        }
        case FieldDescriptor::CPPTYPE_INT32: {
            int32_t v = 0;
            if (!turbo::simple_atoi(value, &v)) {
                return turbo::invalid_argument_error("invalid int32 string");
            }
            if (rep) {
                refl->SetRepeatedInt32(msg, fd, ix, v);
            } else {
                refl->SetInt32(msg, fd, v);
            }
            return turbo::OkStatus();
        }
        case FieldDescriptor::CPPTYPE_UINT32: {
            uint32_t v = 0;
            if (!turbo::simple_atoi(value, &v)) {
                return turbo::invalid_argument_error("invalid uint32 string");
            }
            if (rep) {
                refl->SetRepeatedUInt32(msg, fd, ix, v);
            } else {
                refl->SetUInt32(msg, fd, v);
            }
            return turbo::OkStatus();
        }
        case FieldDescriptor::CPPTYPE_INT64: {
            int64_t v = 0;
            if (!turbo::simple_atoi(value, &v)) {
                return turbo::invalid_argument_error("invalid int64 string");
            }
            if (rep) {
                refl->SetRepeatedInt64(msg, fd, ix, v);
            } else {
                refl->SetInt64(msg, fd, v);
            }
            return turbo::OkStatus();
        }
        case FieldDescriptor::CPPTYPE_UINT64: {
            uint64_t v = 0;
            if (!turbo::simple_atoi(value, &v)) {
                return turbo::invalid_argument_error("invalid uint64 string");
            }
            if (rep) {
                refl->SetRepeatedUInt64(msg, fd, ix, v);
            } else {
                refl->SetUInt64(msg, fd, v);
            }
            return turbo::OkStatus();
        }
        case FieldDescriptor::CPPTYPE_FLOAT: {
            double tmp = 0;
            if (!parse_double_string(value, &tmp)) {
                return turbo::invalid_argument_error("invalid float string");
            }
            const float v = static_cast<float>(tmp);
            if (rep) {
                refl->SetRepeatedFloat(msg, fd, ix, v);
            } else {
                refl->SetFloat(msg, fd, v);
            }
            return turbo::OkStatus();
        }
        case FieldDescriptor::CPPTYPE_DOUBLE: {
            double v = 0;
            if (!parse_double_string(value, &v)) {
                return turbo::invalid_argument_error("invalid double string");
            }
            if (rep) {
                refl->SetRepeatedDouble(msg, fd, ix, v);
            } else {
                refl->SetDouble(msg, fd, v);
            }
            return turbo::OkStatus();
        }
        case FieldDescriptor::CPPTYPE_STRING: {
            const std::string v(value);
            if (rep) {
                refl->SetRepeatedString(msg, fd, ix, v);
            } else {
                refl->SetString(msg, fd, v);
            }
            return turbo::OkStatus();
        }
        case FieldDescriptor::CPPTYPE_ENUM: {
            const auto *ed = fd->enum_type()->FindValueByName(std::string(value));
            if (!ed) {
                return turbo::invalid_argument_error("invalid enum name");
            }
            if (rep) {
                refl->SetRepeatedEnum(msg, fd, ix, ed);
            } else {
                refl->SetEnum(msg, fd, ed);
            }
            return turbo::OkStatus();
        }
        case FieldDescriptor::CPPTYPE_MESSAGE:
            return turbo::invalid_argument_error("message field cannot be set from string");
        default:
            return turbo::invalid_argument_error("unsupported protobuf field type");
    }
}

}  // namespace

turbo::Status FlatProto::set(std::string_view key, const PrimitiveValue &value) {
    if (key.empty()) {
        return turbo::invalid_argument_error("empty flat key");
    }
    TURBO_MOVE_OR_RAISE(const auto parsed, FlatKey::parse_key(key));
    std::vector<AnyUnwindLayer> any_unwind;
    LeafTarget leaf{};
    TURBO_RETURN_NOT_OK(resolve_leaf(_message, parsed, &leaf, &any_unwind));
    TURBO_RETURN_NOT_OK(set_leaf_primitive(leaf, value));
    return repack_any_layers(any_unwind);
}

turbo::Status FlatProto::set(std::string_view key, std::string_view value) {
    if (key.empty()) {
        return turbo::invalid_argument_error("empty flat key");
    }
    TURBO_MOVE_OR_RAISE(const auto parsed, FlatKey::parse_key(key));
    std::vector<AnyUnwindLayer> any_unwind;
    LeafTarget leaf{};
    TURBO_RETURN_NOT_OK(resolve_leaf(_message, parsed, &leaf, &any_unwind));
    TURBO_RETURN_NOT_OK(set_leaf_string(leaf, value));
    return repack_any_layers(any_unwind);
}

}  // namespace merak
