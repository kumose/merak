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

#include <turbo/strings/str_split.h>
#include <turbo/utility/status.h>
#include <turbo/container/flat_hash_map.h>
#include <turbo/container/span.h>

namespace merak {
    class FlatKey {
    public:
        static turbo::Result<std::vector<FlatKey> > parse_key(std::string_view key) {
            std::vector<std::string> pis = turbo::str_split(key, '.', turbo::SkipEmpty());
            if (pis.empty()) {
                return turbo::invalid_argument_error("bad format key of empty string");
            }
            std::vector<FlatKey> result;
            result.reserve(pis.size());
            for (auto it: pis) {
                FlatKey e;
                /// if array set parenet is array
                if (it[0] == '[' && it.size() > 2 && it.back() == ']') {
                    if (result.empty()) {
                        return turbo::invalid_argument_error("bad format key, can not be array at root");
                    }
                    result.back().has_array = true;
                    e.is_index = true;
                    e.key = std::string_view(it.data() + 1, it.size() - 2);
                    if (!turbo::simple_atoi(e.key, &e.index)) {
                        return turbo::invalid_argument_error("bad index key");
                    }
                    result.emplace_back(e);
                    continue;
                }

                e.key = it;
                result.emplace_back(e);
            }
            return result;
        }

        std::string key;
        int index = 0;
        bool has_array{false};
        bool is_index{false};
    };

} // namespace merak
