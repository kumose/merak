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
                if (it[0] == '[') {
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

    template<typename V>
    class FlatTreeNode {
    public:
        using value_type = V;

    public:
        FlatKey key;
        turbo::flat_hash_map<std::string, FlatTreeNode> children;
        value_type value;
        bool is_null{true};
    };

    template<typename V>
    class FlatTree {
    public:
        using value_type = typename std::variant<V, std::vector<V> >;

        turbo::Status feed(turbo::span<std::pair<std::string_view, V>> data) {
            std::vector<std::pair<std::vector<FlatKey>, V>> keyv;
            keyv.reserve(data.size());
            for (auto it: data) {
                TURBO_MOVE_OR_RAISE(auto k, FlatKey::parse_key(it.first));
                keyv.emplace_back(std::pair<std::vector<FlatKey>, V>(k, it.second));
            }
            std::sort(keyv.begin(), keyv.end(),[](const std::pair<std::vector<FlatKey>, V>& r, const std::pair<std::vector<FlatKey>, V> &l) {
                return r.first.size() < l.first.size();
            });
            for (auto it: keyv) {
                TURBO_RETURN_NOT_OK(push_back(it.first, it.second));
            }
            return turbo::OkStatus();
        }

        turbo::Status feed(const turbo::flat_hash_map<std::string_view, V> &data) {
            std::vector<std::pair<std::vector<FlatKey>, V>> keyv;
            keyv.reserve(data.size());
            for (auto it: data) {
                TURBO_MOVE_OR_RAISE(auto k, FlatKey::parse_key(it.first));
                keyv.emplace_back(std::pair<std::vector<FlatKey>, V>(k, it.second));
            }
            std::sort(keyv.begin(), keyv.end(),[](const std::pair<std::vector<FlatKey>, V>& r, const std::pair<std::vector<FlatKey>, V> &l) {
                return r.first.size() < l.first.size();
            });
            for (auto it: keyv) {
                TURBO_RETURN_NOT_OK(push_back(it.first, it.second));
            }
            return turbo::OkStatus();
        }

        /// we assume that std::vector<FlatKey> are sorted, less size should be
        /// insert first
        turbo::Status push_back(std::vector<FlatKey> k,  value_type value) {
            FlatTreeNode<V> node;
            node.key = k.back();
            node.value = value;
            node.is_null = false;
            if (k.size() == 1) {
                root.children[k[0].key] = node;
                return turbo::OkStatus();
            }

            FlatTreeNode<V> *find = nullptr;
            find = &root;
            auto loop = k.size() - 1;
            for (auto i = 0; i < loop; i++) {
                auto tit = find->children.find(k[i].key);
                if (tit != find->children.end()) {
                    find = &tit->second;
                    continue;
                }
                FlatTreeNode<V> n;
                n.key = k[i];
                n.is_null = true;
                find->children[k[i].key] = n;
                tit = find->children.find(k[i].key);
                find = &tit->second;
            }
            find->children[k.back().key] = node;
            return turbo::OkStatus();
        }

    public:
        FlatTreeNode<V> root;
    };
} // namespace merak
