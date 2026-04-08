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
#include <string>

namespace merak {

    class JsonHandlerBase {
    public:
        virtual ~JsonHandlerBase() = default;

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

        virtual void start_array() = 0;

        virtual void end_array(int n = 0) = 0;
    };


    template<typename H>
    class JsonHandlerTemplate : public JsonHandlerBase {
    public:
        JsonHandlerTemplate(H &h) : handler(h) {}
        ~JsonHandlerTemplate() override = default;

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

        void start_array() override {
            handler.start_array();
        }

        void end_array(int n = 0) {
            handler.end_array(n);
        }

        H &handler;
    };
}  // namespace merak
