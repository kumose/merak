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

#include <gtest/gtest.h>
#include <merak/container_traits.h>
#include <vector>
#include <deque>
#include <unordered_map>
#include <turbo/container/flat_hash_map.h>
#include <turbo/container/btree_map.h>
namespace merak {
    TEST(taits, value_type) {
        ASSERT_EQ(merak::internal::HasValueType<std::vector<int>>::value, true);
        ASSERT_EQ(merak::internal::HasValueType<std::deque<int>>::value, true);
        auto v =merak::internal::HasValueType<std::unordered_map<int, int>>::value;
        ASSERT_EQ(v, true);
        v =merak::internal::HasValueType<turbo::flat_hash_map<int, int>>::value;
        ASSERT_EQ(v, true);
        v =merak::internal::HasValueType<turbo::btree_map<int, int>>::value;
        ASSERT_EQ(v, true);
    }

    TEST(taits, key_type) {
        ASSERT_EQ(merak::internal::HasKeyType<std::vector<int>>::value, false);
        ASSERT_EQ(merak::internal::HasKeyType<std::deque<int>>::value, false);
        auto v =merak::internal::HasKeyType<std::unordered_map<int, int>>::value;
        ASSERT_EQ(v, true);
        v =merak::internal::HasKeyType<turbo::flat_hash_map<int, int>>::value;
        ASSERT_EQ(v, true);
        v =merak::internal::HasKeyType<turbo::btree_map<int, int>>::value;
        ASSERT_EQ(v, true);
    }

    TEST(taits, puash_back_type) {
        ASSERT_EQ(merak::internal::HasPushBackType<std::vector<int>>::value, true);
        ASSERT_EQ(merak::internal::HasPushBackType<std::deque<int>>::value, true);
        auto v =merak::internal::HasPushBackType<std::unordered_map<int, int>>::value;
        ASSERT_EQ(v, false);
        v =merak::internal::HasPushBackType<turbo::flat_hash_map<int, int>>::value;
        ASSERT_EQ(v, false);
        v =merak::internal::HasPushBackType<turbo::btree_map<int, int>>::value;
        ASSERT_EQ(v, false);
    }

    TEST(taits, emplace_type) {
        ASSERT_EQ(merak::internal::HasInsertType<std::vector<int>>::value, false);
        ASSERT_EQ(merak::internal::HasInsertType<std::deque<int>>::value, false);
        auto v =merak::internal::HasInsertType<std::unordered_map<int, int>>::value;
        ASSERT_EQ(v, true);
        v =merak::internal::HasInsertType<turbo::flat_hash_map<int, int>>::value;
        ASSERT_EQ(v, true);
        v =merak::internal::HasInsertType<turbo::btree_map<int, int>>::value;
        ASSERT_EQ(v, true);
    }
}
