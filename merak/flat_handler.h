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

#include <string_view>
#include <merak/flat_value.h>
#include <merak/container_traits.h>
#include <turbo/container/flat_hash_map.h>

namespace merak {
    class FlatHandlerBase {
    public:
        virtual ~FlatHandlerBase() = default;

        virtual void start_object() = 0;

        virtual void end_object(int n = 0) = 0;

        virtual void emplace_key(const char *data, size_t size, bool copy) = 0;

        void emplace_key(std::string_view key, bool copy) {
            emplace_key(key.data(), key.size(), copy);
        }

        virtual void emplace_bool(bool value) = 0;

        virtual void emplace_int32(int32_t value) = 0;

        virtual void emplace_uint32(uint32_t value) = 0;

        virtual void emplace_int64(int64_t value) = 0;

        virtual void emplace_uint64(uint64_t value) = 0;

        virtual void emplace_double(double value) = 0;

        virtual void emplace_string(const char *data, size_t len, bool) = 0;
    };

    template<typename C>
    struct ContainsAcceptor {
        using value_type = typename C::value_type;
        static constexpr bool is_map = internal::IsContainsAcceptor<C>::is_map;
        static constexpr bool is_seq = internal::IsContainsAcceptor<C>::is_seq;

        static constexpr bool is_value = std::is_same_v<value_type, std::pair<std::string, PrimitiveValue> > ||  std::is_same_v<value_type, std::pair<const std::string, PrimitiveValue> >;

        static constexpr bool is_string_value = std::is_same_v<value_type, std::pair<std::string, std::string> > || std::is_same_v<value_type, std::pair<const std::string, std::string> >;

        static constexpr bool is_valid () {
            return  (is_map || is_seq) && (is_value || is_string_value);
        }

        static_assert(is_valid(), "value or key type not match");

        ContainsAcceptor(C &c) : _c(c) {
        }

        void handle(value_type &&v) {
            if constexpr (is_map) {
                _c.insert(std::forward<value_type>(v));
            } else {
                _c.push_back(std::forward<value_type>(v));
            }
        }

        template<typename U>
        void handle(std::string_view k, U u) {
            PrimitiveValue v;
            v = u;
            if constexpr (is_value) {
                std::pair<std::string, PrimitiveValue> temp(k, v);
                handle(std::move(temp));
            } else {
                std::pair<std::string, std::string> temp(k, v.to_string());
                handle(std::move(temp));
            }
        }

        C &_c;
    };

    template<typename C>
    class FlatContainerHandler : public FlatHandlerBase {
    public:
        FlatContainerHandler(C &c) : _acceptor(c) {
        }

        void start_object() override {
        }

        void end_object(int n = 0) override {
        }

        void emplace_key(const char *data, size_t size, bool copy) {
            if (!copy) {
                key = std::string_view(data, size);
                return;
            }
            _key.assign(data, size);
            key = _key;
        }

        void emplace_bool(bool value) {
            _acceptor.handle(key, value);
        }

        void emplace_int32(int32_t value) {
            _acceptor.handle(key, value);
        }

        void emplace_uint32(uint32_t value) {
            _acceptor.handle(key, value);
        }

        void emplace_int64(int64_t value) {
            _acceptor.handle(key, value);
        }

        void emplace_uint64(uint64_t value) {
            _acceptor.handle(key, value);
        }

        void emplace_double(double value) {
            _acceptor.handle(key, value);
        }

        void emplace_string(const char *data, size_t len, bool) {
            _acceptor.handle(key, std::string_view(data, len));
        }

    private:
        ContainsAcceptor<C> _acceptor;
        /// no need to copy
        std::string_view key;
        std::string _key;
    };

    template<typename H>
    class FlatHandlerTemplate : public FlatHandlerBase {
    public:
        FlatHandlerTemplate(H &h) : handler(h) {}
        ~FlatHandlerTemplate() override = default;

        void start_object() override {
            handler.start_object();
        }

        void end_object(int n = 0) override {
            handler.end_object(n);
        }

        void emplace_key(const char *data, size_t size, bool copy) override {
            handler.emplace_key(data, size, copy);
        }

        void emplace_bool(bool value) override {
            handler.emplace_bool(value);
        }

        void emplace_int32(int32_t value) override {
            handler.emplace_int32(value);
        }

        void emplace_uint32(uint32_t value) override {
            handler.emplace_uint32(value);
        }

        void emplace_int64(int64_t value) override {
            handler.emplace_int64(value);
        }

        void emplace_uint64(uint64_t value) override {
            handler.emplace_uint64(value);
        }

        void emplace_double(double value) override {
            handler.emplace_double(value);
        }

        void emplace_string(const char *data, size_t len, bool copy) override {
            handler.emplace_string(data, len, copy);
        }
        H &handler;
    };

    inline std::ostream &operator<<(std::ostream &os,
                                    const turbo::flat_hash_map<std::string, PrimitiveValue> &flat_map) {
        for (const auto &kv: flat_map) {
            os << kv.first << "="<<kv.second.to_string()<<"\n";
        }
        return os;
    }

    inline std::ostream &operator<<(std::ostream &os, const turbo::flat_hash_map<std::string, std::string> &flat_map) {
        for (const auto &kv: flat_map) {
            os << kv.first << "=" << kv.second << "\n";
        }
        return os;
    }

} // namespace merak
