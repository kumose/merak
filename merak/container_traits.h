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

#include <merak/flat_value.h>
#include <string>

namespace merak::internal {
    template<typename T, typename = void>
    struct HasValueType : std::false_type {
    };

    template<typename T>
    struct HasValueType<T, std::void_t<typename T::value_type> > : std::true_type {
    };

    template<typename T, typename = void>
    struct HasMappedType : std::false_type {
    };

    template<typename T>
    struct HasMappedType<T, std::void_t<typename T::mapped_type> > : std::true_type {
    };

    template<typename T, typename = void>
    struct HasKeyType : std::false_type {
    };

    template<typename T>
    struct HasKeyType<T, std::void_t<typename T::key_type> > : std::true_type {
    };


    template<typename T, typename = void>
    struct HasPushBackType : std::false_type {
    };

    template<typename T>
    struct HasPushBackType<
                T, std::enable_if_t<
                    std::is_same<decltype(std::declval<T>().push_back(std::declval<typename T::value_type &&>())),
                        typename T::reference>::value ||
                        std::is_same<decltype(std::declval<T>().push_back(std::declval<typename T::value_type &&>())),
                        void>::value
                > >
            : std::true_type {
    };

    template<typename T, typename = void>
    struct HasInsertType : std::false_type {
    };


    template<typename T>
    struct HasInsertType<
                T, std::enable_if_t<
                    std::is_same<decltype(std::declval<T>().insert(std::declval<typename T::value_type &&>())),
                        std::pair<typename T::iterator, bool> >::value
                > >
            : std::true_type {
    };

    template<typename T, typename = void>
    struct IsContainsAcceptor {
        using value_type = void;
        static constexpr bool is_map = false;
        static constexpr bool is_seq = false;
    };

    template<typename T>
    struct IsContainsAcceptor<T, std::enable_if_t<
                HasValueType<T>::value &&
                (HasPushBackType<T>::value ||
                 (HasKeyType<T>::value &&
                  HasMappedType<T>::value &&
                  HasInsertType<T>::value
                 )
                )
            > > {
        using value_type = typename T::value_type;
        static constexpr bool is_map = HasKeyType<T>::value;
        static constexpr bool is_seq = !HasKeyType<T>::value;
    };
} // namespace merak::internal
