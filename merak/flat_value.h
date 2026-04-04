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
#include <optional>
#include <variant>


namespace merak {

    /// do not use std::variant, to control it
    /// more depth.
    enum class PrimitiveValueType  : uint8_t{
        NIL,
        STRING,
        BOOLEAN,
        INT32,
        UINT32,
        INT64,
        UINT64,
        FLOAT,
        DOUBLE
    };

    constexpr std::string_view primitive_value_type_name(PrimitiveValueType type) {
        switch (type) {
            case PrimitiveValueType::STRING:
                return "string";
            case PrimitiveValueType::BOOLEAN:
                return "boolean";
            case PrimitiveValueType::INT32:
                return "int32";
            case PrimitiveValueType::UINT32:
                return "uint32";
            case PrimitiveValueType::INT64:
                return "int64";
            case PrimitiveValueType::UINT64:
                return "uint64";
            case PrimitiveValueType::FLOAT:
                return "float";
            case PrimitiveValueType::DOUBLE:
                return "double";
            default:
                return "nil";
        }
    }

    struct PrimitiveValue {

        struct NilType {};
        using value_type = std::variant<NilType, std::string, bool, int32_t, uint32_t, int64_t, uint64_t, float, double>;

        PrimitiveValue() = default;

        PrimitiveValue &operator=(const PrimitiveValue &v) {
            if (this == &v) {
                return *this;
            }
            this->_type = v._type;
            _data = v._data;
            return *this;
        }

        PrimitiveValue(const PrimitiveValue &v) {
            if (this == &v) {
                return;
            }
            this->_type = v._type;
            _data = v._data;
        }


        PrimitiveValue &operator=(PrimitiveValue &&v) noexcept {
            _data = std::move(v._data);
            _type = v._type;
            return *this;
        }

        PrimitiveValue(PrimitiveValue &&v) noexcept {
            _data = std::move(v._data);
            _type = v._type;
        }


        bool is_string() const {
            return this->_type == PrimitiveValueType::STRING;
        }

        bool is_int32() const {
            return this->_type == PrimitiveValueType::INT32;
        }

        bool is_uint32() const {
            return this->_type == PrimitiveValueType::UINT32;
        }

        bool is_int64() const {
            return this->_type == PrimitiveValueType::INT64;
        }

        bool is_uint64() const {
            return this->_type == PrimitiveValueType::UINT64;
        }

        bool is_float() const {
            return this->_type == PrimitiveValueType::FLOAT;
        }

        bool is_double() const {
            return this->_type == PrimitiveValueType::DOUBLE;
        }

        bool is_nil() const {
            return this->_type == PrimitiveValueType::NIL;
        }

        bool is_boolean() const {
            return this->_type == PrimitiveValueType::BOOLEAN;
        }

        bool is_integer() const {
            return this->_type == PrimitiveValueType::INT32 || this->_type == PrimitiveValueType::UINT32 || this->_type ==
                   PrimitiveValueType::INT64 || this->_type == PrimitiveValueType::UINT64;
        }

        bool is_float_point() const {
            return this->_type == PrimitiveValueType::FLOAT || this->_type == PrimitiveValueType::DOUBLE;
        }

        bool get_bool_confident() const {
            return std::get<bool>(_data);
        }

        int32_t get_int32_confident() const {
            return std::get<int32_t>(_data);
        }

        uint32_t get_uint32_confident() const {
            return std::get<uint32_t>(_data);
        }

        int64_t get_int64_confident() const {
            return std::get<int64_t>(_data);
        }

        uint64_t get_uint64_confident() const {
            return std::get<uint64_t>(_data);
        }

        float get_float_confident() const {
            return std::get<float>(_data);
        }

        double get_double_confident() const {
            return std::get<double>(_data);
        }

        std::string get_string_confident() const &{
            return std::get<std::string>(_data);
        }

        std::string get_string_confident() && {
            _type = PrimitiveValueType::NIL;
            return std::get<std::string>(std::move(_data));
        }

        std::optional<bool> get_bool() const {
            if (_type == PrimitiveValueType::BOOLEAN) {
                return std::get<bool>(_data);
            }
            return std::nullopt;
        }

        template <typename  T>
        std::optional<T> get_value() const {
            if constexpr (std::is_same_v<T, std::string>) {
                return to_string();
            }
            switch (_type) {
                case PrimitiveValueType::BOOLEAN:
                    return static_cast<T>(std::get<bool>(_data));
                case PrimitiveValueType::INT32:
                    return static_cast<T>(std::get<int32_t>(_data));
                case PrimitiveValueType::UINT32:
                    return static_cast<T>(std::get<uint32_t>(_data));
                case PrimitiveValueType::INT64:
                    return static_cast<T>(std::get<int64_t>(_data));
                case PrimitiveValueType::UINT64:
                    return static_cast<T>(std::get<uint64_t>(_data));
                case PrimitiveValueType::FLOAT:
                    return static_cast<T>(std::get<float>(_data));
                case PrimitiveValueType::DOUBLE:
                    return static_cast<T>(std::get<double>(_data));
                default:
                    return std::nullopt;
            }
        }

        std::optional<int32_t> get_int32() const {
            if (_type == PrimitiveValueType::INT32) {
                return std::get<int32_t>(_data);
            }
            return std::nullopt;
        }

        std::optional<uint32_t> get_uint32() const {
            if (_type == PrimitiveValueType::UINT32) {
                return std::get<uint32_t>(_data);
            }
            return std::nullopt;
        }

        std::optional<int64_t> get_int64() const {
            if (_type == PrimitiveValueType::INT64) {
                return std::get<int64_t>(_data);
            }
            return std::nullopt;
        }

        std::optional<uint64_t> get_uint64() const {
            if (_type == PrimitiveValueType::UINT64) {
                return std::get<uint64_t>(_data);
            }
            return std::nullopt;
        }

        std::optional<float> get_float() const {
            if (_type == PrimitiveValueType::FLOAT) {
                return std::get<float>(_data);
            }
            return std::nullopt;
        }

        std::optional<double> get_double() const {
            if (_type == PrimitiveValueType::DOUBLE) {
                return std::get<double>(_data);
            }
            return std::nullopt;
        }

        std::optional<std::string> get_string() const & {
            if (_type == PrimitiveValueType::STRING) {
                return std::get<std::string>(_data);
            }
            return std::nullopt;
        }

        std::optional<std::string> get_string() && {
            if (_type == PrimitiveValueType::STRING) {
                _type = PrimitiveValueType::NIL;
                return std::get<std::string>(std::move(_data));
            }
            return std::nullopt;
        }

        PrimitiveValueType type() const {
            return _type;
        }


        PrimitiveValue &operator=(std::string_view s) {
            _data = std::string(s);
            _type = PrimitiveValueType::STRING;
            return *this;
        }

        PrimitiveValue &operator=(std::string s) {
            _data = std::move(s);
            _type = PrimitiveValueType::STRING;
            return *this;
        }

        PrimitiveValue &operator=(bool b) {
            _data = b;
            _type = PrimitiveValueType::BOOLEAN;
            return *this;
        }

        PrimitiveValue &operator=(int32_t i) {
            _data = i;
            _type = PrimitiveValueType::INT32;
            return *this;
        }

        PrimitiveValue &operator=(uint32_t i) {
            _data = i;
            _type = PrimitiveValueType::UINT32;
            return *this;
        }

        PrimitiveValue &operator=(int64_t i) {
            _data = i;
            _type = PrimitiveValueType::INT64;
            return *this;
        }

        PrimitiveValue &operator=(uint64_t i) {
            _data = i;
            _type = PrimitiveValueType::UINT64;
            return *this;
        }

        PrimitiveValue &operator=(float f) {
            _data = f;
            _type = PrimitiveValueType::FLOAT;
            return *this;
        }

        PrimitiveValue &operator=(double d) {
            _data = d;
            _type = PrimitiveValueType::DOUBLE;
            return *this;
        }

        PrimitiveValue &operator=(std::nullptr_t) {
            _type = PrimitiveValueType::NIL;
            return *this;
        }

        template<typename T, typename std::enable_if<std::is_integral_v<T> && sizeof(T) < sizeof(int), int>::type = 0>
        PrimitiveValue &operator=(T t) {
            if constexpr (std::is_unsigned_v<T>) {
                this->_type = PrimitiveValueType::UINT32;
                _data = static_cast<int32_t>(t);
            } else {
                _data = static_cast<uint32_t>(t);
                this->_type = PrimitiveValueType::INT32;
            }
            return *this;
        }

        std::string to_string() const {
            switch (_type) {
                case PrimitiveValueType::STRING:
                    return std::get<std::string>(_data);
                case PrimitiveValueType::BOOLEAN:
                    return std::get<bool>(_data) ? "true" : "false";
                case PrimitiveValueType::INT32:
                    return std::to_string(std::get<int32_t>(_data));
                case PrimitiveValueType::UINT32:
                    return std::to_string(std::get<uint32_t>(_data));
                case PrimitiveValueType::INT64:
                    return std::to_string(std::get<int64_t>(_data));
                case PrimitiveValueType::UINT64:
                    return std::to_string(std::get<uint64_t>(_data));
                case PrimitiveValueType::FLOAT:
                    return std::to_string(std::get<float>(_data));
                case PrimitiveValueType::DOUBLE:
                    return std::to_string(std::get<double>(_data));
                default:
                    return "nil";
            }
        }
    private:
        PrimitiveValueType _type{PrimitiveValueType::NIL};
        value_type _data;
    };

    inline std::ostream &operator<<(std::ostream &os, const PrimitiveValue &value) {
        os << value.to_string();
        return os;
    }
} // namespace merak
