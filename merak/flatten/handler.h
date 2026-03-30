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
#include <turbo/container/flat_hash_map.h>
#include <merak/options.h>

namespace merak {


    using FlatValueType = std::variant<std::string, bool, int32_t, uint32_t, int64_t, uint64_t, float, double>;
    using FlatValueMap = turbo::flat_hash_map<std::string, FlatValueType>;


    class FlatHandler {
    public:
        virtual ~FlatHandler() = default;

        virtual void start_object() = 0;

        virtual void end_object(int n = 0) = 0;

        virtual void emplace_key(const char *data, size_t size, bool copy) = 0;

        void emplace_key(std::string_view key, bool copy) {
            emplace_key(key.data(),key.size(), copy);
        }

        virtual void emplace_bool(bool value) = 0;

        virtual void emplace_int32(int32_t value) = 0;

        virtual void emplace_uint32(uint32_t value) = 0;

        virtual void emplace_int64(int64_t value) = 0;

        virtual void emplace_uint64(uint64_t value) = 0;

        virtual void emplace_double(double value) = 0;

        virtual void emplace_string(const char *data, size_t len, bool) = 0;
    };

    class FlatMapHandler : public FlatHandler {
    public:
        FlatMapHandler() = default;

        ~FlatMapHandler() = default;

        void start_object() {
        }

        void end_object(int n = 0) {
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
            FlatValueType v = value;
            flatmap[key] = std::move(v);
        }

        void emplace_int32(int32_t value) {
            FlatValueType v = value;
            flatmap[key] = std::move(v);
        }

        void emplace_uint32(uint32_t value) {
            FlatValueType v = value;
            flatmap[key] = std::move(v);
        }

        void emplace_int64(int64_t value) {
            FlatValueType v = value;
            flatmap[key] = std::move(v);
        }

        void emplace_uint64(uint64_t value) {
            FlatValueType v = value;
            flatmap[key] = std::move(v);
        }

        void emplace_double(double value) {
            FlatValueType v = value;
            flatmap[key] = std::move(v);
        }

        void emplace_string(const char *data, size_t len, bool) {
            FlatValueType value;
            value = std::string(data, len);
            flatmap[key] = std::move(value);
        }

        turbo::flat_hash_map<std::string, FlatValueType> flatmap;

        /// no need to copy
        std::string_view key;
        std::string _key;
    };

    class FlatStringMapHandler : public FlatHandler  {
    public:
        FlatStringMapHandler() = default;

        ~FlatStringMapHandler() = default;

        void start_object() {
        }

        void end_object(int n = 0) {
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
            if (value)
                flatmap[key] = "true";
            else
                flatmap[key] = "false";
        }

        void emplace_int32(int32_t value) {
            flatmap[key] = turbo::str_cat(value);
        }

        void emplace_uint32(uint32_t value) {
            flatmap[key] = turbo::str_cat(value);
        }

        void emplace_int64(int64_t value) {
            flatmap[key] = turbo::str_cat(value);
        }

        void emplace_uint64(uint64_t value) {
            flatmap[key] = turbo::str_cat(value);
        }

        void emplace_double(double value) {
            flatmap[key] = turbo::str_cat(value);
        }

        void emplace_string(const char *data, size_t len, bool) {
            flatmap[key] = std::move(std::string(data, len));

        }

        turbo::flat_hash_map<std::string, std::string> flatmap;

        /// no need to copy
        std::string_view key;
        std::string _key;
    };

    template<typename H>
    class FlatHandlerTemplate : public FlatHandler {
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

        void emplace_string(const char *data, size_t len, bool) override {
            handler.emplace_string(data, len);
        }
        H &handler;
    };

    inline std::ostream &operator<<(std::ostream &os,
                                    const turbo::flat_hash_map<std::string, FlatValueType> &flat_map) {
        for (const auto &kv: flat_map) {
            os << kv.first << "=";

            std::visit([&os](auto &&arg) {
                using T = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<T, std::string>) {
                    os << arg;
                }
                // bool  true / false
                else if constexpr (std::is_same_v<T, bool>) {
                    os << std::boolalpha << arg;
                } else {
                    os << turbo::str_cat(arg);
                }
            }, kv.second);

            os << "\n";
        }
        return os;
    }

    inline std::ostream &operator<<(std::ostream &os, const turbo::flat_hash_map<std::string, std::string> &flat_map) {
        for (const auto &kv: flat_map) {
            os << kv.first << "=" << kv.second << "\n";
        }
        return os;
    }
}  // namespace merak

