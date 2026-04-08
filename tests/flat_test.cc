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
#include <algorithm>
#include <vector>
#include <sstream>
#include <merak/flatten.h>
#include <tests/proto/flat2_test.pb.h>
#include <tests/proto/flat3_test.pb.h>

namespace merak {
namespace {

[[maybe_unused]] std::string flat_map_to_string(const turbo::flat_hash_map<std::string, std::string>& flat) {
    std::vector<std::pair<std::string, std::string>> ordered(flat.begin(), flat.end());
    std::sort(ordered.begin(), ordered.end(), [](const auto& l, const auto& r) {
        return l.first < r.first;
    });
    std::ostringstream os;
    os << "flat_map(size=" << ordered.size() << ")\n";
    for (const auto& kv : ordered) {
        os << "  " << kv.first << " = " << kv.second << "\n";
    }
    return os.str();
}

TEST(FlatTest, Proto2ToFlatMap) {
    flat2::FlatTest msg;
    msg.add_names("alice");
    msg.add_names("bob");
    msg.set_id(42);
    msg.add_array(101);
    msg.add_array(202);

    turbo::flat_hash_map<std::string, std::string> flat;
    ASSERT_TRUE(proto_message_to_flat(msg, flat).ok());

    ASSERT_EQ(flat["names.[0]"], "alice") << flat_map_to_string(flat);
    ASSERT_EQ(flat["names.[1]"], "bob") << flat_map_to_string(flat);
    ASSERT_EQ(flat["id"], "42") << flat_map_to_string(flat);
    ASSERT_EQ(flat["array.[0]"], "101") << flat_map_to_string(flat);
    ASSERT_EQ(flat["array.[1]"], "202") << flat_map_to_string(flat);
}

TEST(FlatTest, Proto3ToFlatMap) {
    flat3::FlatTest msg;
    msg.add_names("tom");
    msg.add_names("jerry");
    msg.set_id(7);
    msg.add_array(11);
    msg.add_array(22);

    Pb2FlatOptions op;
    op.always_print_primitive_fields = true;

    turbo::flat_hash_map<std::string, std::string> flat;
    ASSERT_TRUE(proto_message_to_flat(msg, flat, op).ok());

    ASSERT_EQ(flat["names.[0]"], "tom") << flat_map_to_string(flat);
    ASSERT_EQ(flat["names.[1]"], "jerry") << flat_map_to_string(flat);
    ASSERT_EQ(flat["id"], "7") << flat_map_to_string(flat);
    ASSERT_EQ(flat["array.[0]"], "11") << flat_map_to_string(flat);
    ASSERT_EQ(flat["array.[1]"], "22") << flat_map_to_string(flat);
}

TEST(FlatTest, JsonToFlatMapForProtoShape) {
    const std::string json = R"({"names":["u1","u2"],"id":5,"array":[9,10]})";

    turbo::flat_hash_map<std::string, std::string> flat;
    auto rs = json_to_flat_map(json, flat);
    ASSERT_TRUE(rs.ok())<<rs.to_string();

    ASSERT_EQ(flat["names.[0]"], "u1") << flat_map_to_string(flat);
    ASSERT_EQ(flat["names.[1]"], "u2") << flat_map_to_string(flat);
    ASSERT_EQ(flat["id"], "5") << flat_map_to_string(flat);
    ASSERT_EQ(flat["array.[0]"], "9") << flat_map_to_string(flat);
    ASSERT_EQ(flat["array.[1]"], "10") << flat_map_to_string(flat);
}

}  // namespace
}  // namespace merak
