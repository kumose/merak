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

#include <type_traits>

#include <turbo/utility/status.h>
#include <google/protobuf/message.h>
#include <merak/flat_value.h>
#include <merak/flatten/flat_key.h>
#include <merak/container_traits.h>

namespace merak {

    /// Writes dotted paths into a \c google::protobuf::Message using the same rules as
    /// \c FlatKey::parse_key and \c proto_message_to_flat: each segment is a field name from the
    /// current message's \c Descriptor, plus \c .[index] for repeated fields and map-key segments
    /// after map field names as documented there.
    ///
    /// Typical usage: build or load a \c Message, construct \c FlatProto on it, then \c set each
    /// entry from configuration or from a flat map. String proto fields accept \c set(key, string_view);
    /// other scalars use \c PrimitiveValue. The overload \c set(container) iterates \c pair paths;
    /// the container must be a map or sequence type accepted by \c IsContainsAcceptor (see
    /// \c static_assert in the template).
    ///
    /// There is no \c get: random access by flat string key would be expensive; read via
    /// \c proto_message_to_flat into a hash map when you need lookup.
    ///
    /// Message fields are traversed until a scalar/string/bytes leaf. For \c google::protobuf::Any,
    /// \c proto_message_to_flat always emits \c type_url (or \c @type per options) plus \c value as a
    /// \b base64 string of the wire payload (no unpacked inner keys). For \c set, a path ending in
    /// \c ...value on \c google::protobuf::Any \b must be valid base64 (decoding errors fail the \c set).
    /// Other segments still follow \c TYPE_URL, \c A_TYPE_URL, \c VALUE_NAME in \c merak/proto/descriptor.h;
    /// you may set inner field paths after the type URL
    /// (unpacked edit); each successful \c set re-packs the Any, and the type URL must be set before
    /// addressing inner fields when using inner paths.
    ///
    /// \b Style: flat KV is meant to be human-readable; \c Any carries an opaque binary \c value on the
    /// wire, so \b prefer concrete message types over \c Any when config is authored as string key-value
    /// maps. Even when inner fields look readable after unpack, consuming them still needs the inner type
    /// registered (generated \c Message or descriptor pool)—fine for a hop that forwards opaque payloads,
    /// but typical business components do not all link every \c .proto. Prefer fixed sub-messages in shared
    /// schemas so consumers do not need ad-hoc PB integration per \c type_url. This API still supports \c Any
    /// as above; we do not disallow it.
    ///
    /// Errors are returned as \c turbo::Status: for example unknown field, bad key shape,
    /// path/type mismatch (e.g. index on non-repeated field), empty Any \c type_url when addressing
    /// inner fields, unknown \c type_url type name, invalid base64 for Any \c value, invalid serialized
    /// inner payload, or assigning a scalar to a non-leaf message field.
    ///
    /// Repeated fields: callers often cannot control application order. Keys may arrive with a larger
    /// index before a smaller one (e.g. hash-map iteration), and \c set calls may be interleaved with
    /// unrelated fields—you might set \c items.[2], then other paths, then come back to \c items.[0].
    /// That is supported: each indexed write extends the repeated field as needed (padding with
    /// default-valued elements), so no prior explicit \c resize is required. Indices that were never
    /// written keep those defaults; only set indices you intend to be real values.
    class FlatProto {
    public:
    explicit FlatProto(google::protobuf::Message &message) : _message(message) {
    }
    ~FlatProto() = default;

    /// Set a scalar or enum-compatible value at \p key. Nil clears optional singular fields;
    /// it is an error on a repeated element.
    turbo::Status set(std::string_view key, const PrimitiveValue &value);

    /// Set a string field (including bytes as UTF-8 string) at \p key.
    turbo::Status set(std::string_view key, std::string_view value);

    /// Batch \c set from a map or sequence of path/value pairs. Order follows container iteration;
    /// repeated keys need not be sorted; see class comment.
    template<typename C>
    turbo::Status set(const C &container) {
        using value_type = typename C::value_type;
        static constexpr bool is_map = internal::IsContainsAcceptor<C>::is_map;
        static constexpr bool is_seq = internal::IsContainsAcceptor<C>::is_seq;

        static constexpr bool is_value = std::is_same_v<value_type, std::pair<std::string, PrimitiveValue> > ||  std::is_same_v<value_type, std::pair<const std::string, PrimitiveValue> >;

        static constexpr bool is_string_value = std::is_same_v<value_type, std::pair<std::string, std::string> > || std::is_same_v<value_type, std::pair<const std::string, std::string> >;

        static constexpr bool k_valid = (is_map || is_seq) && (is_value || is_string_value);

        static_assert(k_valid, "value or key type not match");
        for(auto &item : container) {
            TURBO_RETURN_NOT_OK(set(item.first, item.second));
        }
        return turbo::OkStatus();
    }

    private:
        google::protobuf::Message &_message;
    };
}  // namespace merak
