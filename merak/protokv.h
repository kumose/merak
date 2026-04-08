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

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include <google/protobuf/message.h>
#include <turbo/container/flat_hash_map.h>
#include <turbo/utility/status.h>

#include <merak/flat_value.h>
#include <merak/flatten/flat_pb.h>
#include <merak/flatten/pb_to_flat.h>
#include <merak/options.h>

namespace merak {

    // ProtoKv is a template helper with a minimal default; change members, add APIs, or specialize as
    // needed for your project.
    //
    // Optional / unset fields: under default proto_message_to_flat rules, unset optionals (and other
    // leaves that are not emitted) do not appear in kv_. get(path) returns std::nullopt until you
    // set/apply, the field exists on the message and you refresh_kv_from_proto, or you pass
    // Pb2FlatOptions on refresh (e.g. always_print_primitive_fields) to fold defaults into the map —
    // see merak/options.h (that option affects more than primitive singular fields).

    namespace protokv_detail {
        template<typename V>
        inline constexpr bool is_kv_value_v =
            std::is_same_v<std::remove_cv_t<std::remove_reference_t<V>>, PrimitiveValue> ||
            std::is_same_v<std::remove_cv_t<std::remove_reference_t<V>>, std::string>;
    }  // namespace protokv_detail

    /// Owns a concrete protobuf \c M and a flat \c path -> \c V map, where \c V is \c PrimitiveValue
    /// (typed scalars) or \c std::string (string form as in \c proto_message_to_flat's string map).
    /// Default \c V is \c PrimitiveValue (\c ProtoKv<M>).
    ///
    /// After changing \c proto() outside this class, call \c refresh_kv_from_proto. \c get reads only
    /// from \c kv_ (refresh first if the message changed). \c apply_kv_to_proto batch-applies \c kv_
    /// via \c FlatProto::set. Erasing keys from \c kv alone does not clear fields on the message.
    template<typename M, typename V = PrimitiveValue>
    class ProtoKv {
        static_assert(std::is_base_of_v<google::protobuf::Message, M>, "ProtoKv: M must be a protobuf Message");
        static_assert(protokv_detail::is_kv_value_v<V>, "ProtoKv: V must be PrimitiveValue or std::string");

    public:
        ProtoKv() : message_(std::make_unique<M>()) {}

        explicit ProtoKv(std::unique_ptr<M> message)
                : message_(message ? std::move(message) : std::make_unique<M>()) {}

        ProtoKv(const ProtoKv &) = delete;

        ProtoKv &operator=(const ProtoKv &) = delete;

        ProtoKv(ProtoKv &&) noexcept = default;

        ProtoKv &operator=(ProtoKv &&) noexcept = default;

        M &proto() { return *message_; }

        const M &proto() const { return *message_; }

        turbo::flat_hash_map<std::string, V> &kv() { return kv_; }

        const turbo::flat_hash_map<std::string, V> &kv() const { return kv_; }

        /// O(1) lookup in \c kv_. Does not read from \c proto(); use \c refresh_kv_from_proto if needed.
        std::optional<V> get(std::string_view key) const {
            auto it = kv_.find(std::string(key));
            if (it == kv_.end()) {
                return std::nullopt;
            }
            return it->second;
        }

        turbo::Status refresh_kv_from_proto(const Pb2FlatOptions &op = Pb2FlatOptions()) {
            kv_.clear();
            return proto_message_to_flat(*message_, kv_, op);
        }

        turbo::Status apply_kv_to_proto() {
            FlatProto fp(*message_);
            return fp.set(kv_);
        }

        template<typename Vv = V, typename = std::enable_if_t<std::is_same_v<Vv, PrimitiveValue>>>
        turbo::Status set(std::string_view key, const PrimitiveValue &value) {
            FlatProto fp(*message_);
            TURBO_RETURN_NOT_OK(fp.set(key, value));
            kv_[std::string(key)] = value;
            return turbo::OkStatus();
        }

        turbo::Status set(std::string_view key, std::string_view value) {
            FlatProto fp(*message_);
            TURBO_RETURN_NOT_OK(fp.set(key, value));
            if constexpr (std::is_same_v<V, std::string>) {
                kv_.insert_or_assign(std::string(key), std::string(value));
            } else {
                PrimitiveValue pv;
                pv = value;
                kv_[std::string(key)] = std::move(pv);
            }
            return turbo::OkStatus();
        }

        FlatProto flat_proto() { return FlatProto(*message_); }

    private:
        std::unique_ptr<M> message_;
        turbo::flat_hash_map<std::string, V> kv_;
    };

}  // namespace merak
