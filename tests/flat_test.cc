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
#include <unordered_map>
#include <merak/flatten.h>
#include <merak/flatten/flat_pb.h>
#include <merak/proto/descriptor.h>
#include <turbo/strings/escaping.h>
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

/// \c flat[k] would default-insert; unset optionals are not exported—use \c find to assert presence.
[[maybe_unused]] static void assert_flat_has_non_empty(
        const turbo::flat_hash_map<std::string, std::string> &flat,
        const std::string &key) {
    auto it = flat.find(key);
    ASSERT_NE(it, flat.end())
            << "key not exported (unset optional or path mismatch): " << key << "\n"
            << flat_map_to_string(flat);
    ASSERT_FALSE(it->second.empty())
            << "empty value for key: " << key << "\n"
            << flat_map_to_string(flat);
}

[[maybe_unused]] static void assert_flat_eq(
        const turbo::flat_hash_map<std::string, std::string> &flat,
        const std::string &key,
        const std::string &expected) {
    auto it = flat.find(key);
    ASSERT_NE(it, flat.end())
            << "missing key: " << key << "\n"
            << flat_map_to_string(flat);
    ASSERT_EQ(it->second, expected) << flat_map_to_string(flat);
}

[[maybe_unused]] std::string seq_to_string(const std::vector<std::pair<std::string, std::string>>& flat) {
    std::ostringstream os;
    os << "flat_seq(size=" << flat.size() << ")\n";
    for (const auto& kv : flat) {
        os << "  " << kv.first << " = " << kv.second << "\n";
    }
    return os.str();
}

[[maybe_unused]] std::string find_in_seq(
        const std::vector<std::pair<std::string, std::string>>& flat, const std::string& key) {
    for (const auto& kv : flat) {
        if (kv.first == key) {
            return kv.second;
        }
    }
    return "";
}

[[maybe_unused]] static bool ends_with(std::string_view s, std::string_view suffix) {
    return s.size() >= suffix.size() &&
           s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

/// When batch-applying flat keys, sort so segments ending in \c TYPE_URL / \c A_TYPE_URL (see
/// \c descriptor.h) run before paths into the unpacked Any payload (prefix is whatever your proto
/// names the Any field; inner tail comes from the inner \c Descriptor). Lexicographic order can
/// otherwise apply payload keys before the type-URL key.
[[maybe_unused]] static void sort_flat_keys_for_flatproto_apply(
        std::vector<std::pair<std::string, std::string>> *entries) {
    static const std::string dot_type_url =
            std::string(".") + std::string(TYPE_URL);
    static const std::string dot_a_type_url =
            std::string(".") + std::string(A_TYPE_URL);
    static const std::string dot_value =
            std::string(".") + std::string(VALUE_NAME);
    std::sort(entries->begin(), entries->end(), [&](const auto &l, const auto &r) {
        auto pri = [&](std::string_view k) -> int {
            if (ends_with(k, dot_type_url) || ends_with(k, dot_a_type_url)) {
                return 0;
            }
            if (ends_with(k, dot_value)) {
                return 1;
            }
            return 2;
        };
        const int pl = pri(l.first);
        const int pr = pri(r.first);
        if (pl != pr) {
            return pl < pr;
        }
        return l.first < r.first;
    });
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

TEST(FlatTest, Proto2WithAnyToFlatMap) {
    flat2::FlatTest msg;
    msg.add_names("x");
    msg.set_id(1);
    msg.add_array(2);
    flat2::FlatInner inner;
    inner.set_a(55);
    ASSERT_TRUE(msg.mutable_detail()->PackFrom(inner));

    turbo::flat_hash_map<std::string, std::string> flat;
    ASSERT_TRUE(proto_message_to_flat(msg, flat).ok());
    assert_flat_has_non_empty(flat, "detail.type_url");
    std::string expected_b64;
    turbo::base64_encode(msg.detail().value(), &expected_b64);
    assert_flat_eq(flat, "detail.value", expected_b64);
    EXPECT_EQ(flat.find("detail.a"), flat.end());
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
    auto rs_flat = proto_message_to_flat(msg, flat, op);
    ASSERT_TRUE(rs_flat.ok()) << rs_flat.to_string();

    ASSERT_EQ(flat["names.[0]"], "tom") << flat_map_to_string(flat);
    ASSERT_EQ(flat["names.[1]"], "jerry") << flat_map_to_string(flat);
    ASSERT_EQ(flat["id"], "7") << flat_map_to_string(flat);
    ASSERT_EQ(flat["array.[0]"], "11") << flat_map_to_string(flat);
    ASSERT_EQ(flat["array.[1]"], "22") << flat_map_to_string(flat);
}

TEST(FlatTest, Proto3WithAnyToFlatMap) {
    flat3::FlatTest msg;
    msg.add_names("p");
    msg.set_id(2);
    msg.add_array(3);
    flat3::FlatInner inner;
    inner.set_a(66);
    ASSERT_TRUE(msg.mutable_detail()->PackFrom(inner));

    Pb2FlatOptions op;
    op.always_print_primitive_fields = true;

    turbo::flat_hash_map<std::string, std::string> flat;
    ASSERT_TRUE(proto_message_to_flat(msg, flat, op).ok());
    assert_flat_has_non_empty(flat, "detail.type_url");
    std::string expected_b64;
    turbo::base64_encode(msg.detail().value(), &expected_b64);
    assert_flat_eq(flat, "detail.value", expected_b64);
    EXPECT_EQ(flat.find("detail.a"), flat.end());
}

TEST(FlatTest, Proto3UnsetAnyToFlatWithAlwaysPrintPrimitiveFields) {
    flat3::FlatTest msg;
    msg.set_id(1);

    Pb2FlatOptions op;
    op.always_print_primitive_fields = true;

    turbo::flat_hash_map<std::string, std::string> flat;
    auto rs = proto_message_to_flat(msg, flat, op);
    ASSERT_TRUE(rs.ok()) << rs.to_string();

    auto it = flat.find("detail.type_url");
    ASSERT_NE(it, flat.end());
    EXPECT_TRUE(it->second.empty());
    auto vit = flat.find("detail.value");
    ASSERT_NE(vit, flat.end());
    EXPECT_TRUE(vit->second.empty());
    EXPECT_EQ(flat.find("detail.a"), flat.end());
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

TEST(FlatTest, JsonToFlatVectorPairContainer) {
    const std::string json = R"({"names":["u1","u2"],"id":5,"array":[9,10]})";
    std::vector<std::pair<std::string, std::string>> flat;
    FlatContainerHandler<std::vector<std::pair<std::string, std::string>>> handler(flat);
    auto rs = json_to_flat(json, &handler);
    ASSERT_TRUE(rs.ok()) << rs.to_string();

    ASSERT_EQ(find_in_seq(flat, "names.[0]"), "u1") << seq_to_string(flat);
    ASSERT_EQ(find_in_seq(flat, "names.[1]"), "u2") << seq_to_string(flat);
    ASSERT_EQ(find_in_seq(flat, "id"), "5") << seq_to_string(flat);
    ASSERT_EQ(find_in_seq(flat, "array.[0]"), "9") << seq_to_string(flat);
    ASSERT_EQ(find_in_seq(flat, "array.[1]"), "10") << seq_to_string(flat);
}

TEST(FlatTest, JsonToFlatUnorderedMapContainer) {
    const std::string json = R"({"names":["u1","u2"],"id":5,"array":[9,10]})";
    std::unordered_map<std::string, std::string> flat;
    FlatContainerHandler<std::unordered_map<std::string, std::string>> handler(flat);
    auto rs = json_to_flat(json, &handler);
    ASSERT_TRUE(rs.ok()) << rs.to_string();

    ASSERT_EQ(flat["names.[0]"], "u1");
    ASSERT_EQ(flat["names.[1]"], "u2");
    ASSERT_EQ(flat["id"], "5");
    ASSERT_EQ(flat["array.[0]"], "9");
    ASSERT_EQ(flat["array.[1]"], "10");
}

TEST(FlatProtoTest, RepeatedStringOutOfOrder) {
    flat3::FlatTest msg;
    FlatProto fp(msg);
    ASSERT_TRUE(fp.set("names.[2]", std::string_view{"c"}).ok());
    ASSERT_TRUE(fp.set("names.[0]", std::string_view{"a"}).ok());
    ASSERT_TRUE(fp.set("names.[1]", std::string_view{"b"}).ok());
    ASSERT_EQ(msg.names_size(), 3);
    EXPECT_EQ(msg.names(0), "a");
    EXPECT_EQ(msg.names(1), "b");
    EXPECT_EQ(msg.names(2), "c");
}

TEST(FlatProtoTest, RepeatedInt64InterleavedWithOtherField) {
    flat3::FlatTest msg;
    FlatProto fp(msg);
    PrimitiveValue v64;
    v64 = int64_t{200};
    ASSERT_TRUE(fp.set("array.[1]", v64).ok());
    PrimitiveValue idv;
    idv = int32_t{88};
    ASSERT_TRUE(fp.set("id", idv).ok());
    v64 = int64_t{100};
    ASSERT_TRUE(fp.set("array.[0]", v64).ok());
    EXPECT_EQ(msg.id(), 88);
    ASSERT_EQ(msg.array_size(), 2);
    EXPECT_EQ(msg.array(0), 100);
    EXPECT_EQ(msg.array(1), 200);
}

TEST(FlatProtoTest, SetContainerStringMapUnordered) {
    flat3::FlatTest msg;
    std::unordered_map<std::string, std::string> updates;
    updates["names.[2]"] = "z";
    updates["id"] = "5";
    updates["names.[0]"] = "x";
    updates["array.[1]"] = "22";
    updates["names.[1]"] = "y";
    updates["array.[0]"] = "11";
    FlatProto fp(msg);
    ASSERT_TRUE(fp.set(updates).ok());
    EXPECT_EQ(msg.id(), 5);
    ASSERT_EQ(msg.names_size(), 3);
    EXPECT_EQ(msg.names(0), "x");
    EXPECT_EQ(msg.names(1), "y");
    EXPECT_EQ(msg.names(2), "z");
    ASSERT_EQ(msg.array_size(), 2);
    EXPECT_EQ(msg.array(0), 11);
    EXPECT_EQ(msg.array(1), 22);

    turbo::flat_hash_map<std::string, std::string> flat;
    Pb2FlatOptions op;
    op.always_print_primitive_fields = true;
    auto rs = proto_message_to_flat(msg, flat, op);
    ASSERT_TRUE(rs.ok())<<rs.to_string();
    EXPECT_EQ(flat["names.[0]"], "x");
    EXPECT_EQ(flat["names.[1]"], "y");
    EXPECT_EQ(flat["names.[2]"], "z");
    EXPECT_EQ(flat["id"], "5");
    EXPECT_EQ(flat["array.[0]"], "11");
    EXPECT_EQ(flat["array.[1]"], "22");
}

TEST(FlatProtoTest, Proto2OptionalIdWithRepeated) {
    flat2::FlatTest msg;
    FlatProto fp(msg);
    ASSERT_TRUE(fp.set("names.[1]", std::string_view{"second"}).ok());
    PrimitiveValue idv;
    idv = int32_t{7};
    ASSERT_TRUE(fp.set("id", idv).ok());
    ASSERT_TRUE(fp.set("names.[0]", std::string_view{"first"}).ok());
    ASSERT_TRUE(msg.has_id());
    EXPECT_EQ(msg.id(), 7);
    ASSERT_EQ(msg.names_size(), 2);
    EXPECT_EQ(msg.names(0), "first");
    EXPECT_EQ(msg.names(1), "second");
}

TEST(FlatProtoTest, Flat2DetailAnyInnerField) {
    flat2::FlatTest msg;
    FlatProto fp(msg);
    ASSERT_TRUE(fp.set("detail.type_url", std::string_view{"flat2.FlatInner"}).ok());
    PrimitiveValue v;
    v = int64_t{30};
    ASSERT_TRUE(fp.set("detail.a", v).ok());
    ASSERT_TRUE(msg.has_detail());
    flat2::FlatInner inner;
    ASSERT_TRUE(msg.detail().UnpackTo(&inner));
    EXPECT_EQ(inner.a(), 30);
}

TEST(FlatProtoTest, Flat3DetailAnyInnerField) {
    flat3::FlatTest msg;
    FlatProto fp(msg);
    ASSERT_TRUE(fp.set("detail.type_url", std::string_view{"flat3.FlatInner"}).ok());
    PrimitiveValue v;
    v = int64_t{40};
    ASSERT_TRUE(fp.set("detail.a", v).ok());
    flat3::FlatInner inner;
    ASSERT_TRUE(msg.detail().UnpackTo(&inner));
    EXPECT_EQ(inner.a(), 40);
}

TEST(FlatProtoTest, Flat2DetailAnyInnerWithATypeSegment) {
    flat2::FlatTest msg;
    FlatProto fp(msg);
    ASSERT_TRUE(fp.set("detail.@type", std::string_view{"flat2.FlatInner"}).ok());
    ASSERT_TRUE(fp.set("detail.a", std::string_view{"100"}).ok());
    flat2::FlatInner inner;
    ASSERT_TRUE(msg.detail().UnpackTo(&inner));
    EXPECT_EQ(inner.a(), 100);
}

TEST(FlatProtoTest, Flat2DetailAnyInnerWithoutTypeUrlFails) {
    flat2::FlatTest msg;
    FlatProto fp(msg);
    PrimitiveValue pv;
    pv = int64_t{1};
    auto st = fp.set("detail.a", pv);
    ASSERT_FALSE(st.ok());
}

TEST(FlatProtoTest, Flat2RepeatedDetailsAnyInner) {
    flat2::FlatTest msg;
    FlatProto fp(msg);
    ASSERT_TRUE(fp.set("details.[0].type_url", std::string_view{"flat2.FlatInner"}).ok());
    PrimitiveValue v;
    v = int64_t{9};
    ASSERT_TRUE(fp.set("details.[0].a", v).ok());
    ASSERT_EQ(msg.details_size(), 1);
    flat2::FlatInner inner;
    ASSERT_TRUE(msg.details(0).UnpackTo(&inner));
    EXPECT_EQ(inner.a(), 9);
}

TEST(FlatProtoTest, Flat2DetailAnyInterleavedWithId) {
    flat2::FlatTest msg;
    FlatProto fp(msg);
    ASSERT_TRUE(fp.set("detail.type_url", std::string_view{"flat2.FlatInner"}).ok());
    PrimitiveValue v;
    v = int64_t{3};
    ASSERT_TRUE(fp.set("detail.a", v).ok());
    v = int32_t{21};
    ASSERT_TRUE(fp.set("id", v).ok());
    v = int64_t{5};
    ASSERT_TRUE(fp.set("detail.a", v).ok());
    flat2::FlatInner inner;
    ASSERT_TRUE(msg.detail().UnpackTo(&inner));
    EXPECT_EQ(inner.a(), 5);
    ASSERT_TRUE(msg.has_id());
    EXPECT_EQ(msg.id(), 21);
}

TEST(FlatProtoTest, Flat3RepeatedDetailsAnyInner) {
    flat3::FlatTest msg;
    FlatProto fp(msg);
    ASSERT_TRUE(fp.set("details.[0].type_url", std::string_view{"flat3.FlatInner"}).ok());
    PrimitiveValue v;
    v = int64_t{17};
    ASSERT_TRUE(fp.set("details.[0].a", v).ok());
    ASSERT_EQ(msg.details_size(), 1);
    flat3::FlatInner inner;
    ASSERT_TRUE(msg.details(0).UnpackTo(&inner));
    EXPECT_EQ(inner.a(), 17);
}

TEST(FlatProtoTest, Flat2RoundTripDetailFlatMapWithATypeUrlKeys) {
    flat2::FlatTest src;
    flat2::FlatInner packed;
    packed.set_a(11);
    ASSERT_TRUE(src.mutable_detail()->PackFrom(packed));
    Pb2FlatOptions gen;
    gen.using_a_type_url = true;
    turbo::flat_hash_map<std::string, std::string> flat;
    ASSERT_TRUE(proto_message_to_flat(src, flat, gen).ok());
    assert_flat_has_non_empty(flat, std::string("detail.") + std::string(A_TYPE_URL));
    std::string b64_roundtrip;
    turbo::base64_encode(src.detail().value(), &b64_roundtrip);
    assert_flat_eq(flat, "detail.value", b64_roundtrip);

    flat2::FlatTest dst;
    FlatProto fp(dst);
    std::vector<std::pair<std::string, std::string>> ordered(flat.begin(), flat.end());
    sort_flat_keys_for_flatproto_apply(&ordered);
    for (const auto &kv : ordered) {
        ASSERT_TRUE(fp.set(kv.first, kv.second).ok()) << kv.first;
    }
    flat2::FlatInner out;
    ASSERT_TRUE(dst.detail().UnpackTo(&out));
    EXPECT_EQ(out.a(), 11);
}

TEST(FlatProtoTest, Flat2RoundTripDetailFlatMapWithTypeUrlKeys) {
    flat2::FlatTest src;
    flat2::FlatInner packed;
    packed.set_a(13);
    ASSERT_TRUE(src.mutable_detail()->PackFrom(packed));
    Pb2FlatOptions gen;
    turbo::flat_hash_map<std::string, std::string> flat;
    ASSERT_TRUE(proto_message_to_flat(src, flat, gen).ok());
    assert_flat_has_non_empty(flat, std::string("detail.") + std::string(TYPE_URL));
    std::string b64_roundtrip2;
    turbo::base64_encode(src.detail().value(), &b64_roundtrip2);
    assert_flat_eq(flat, "detail.value", b64_roundtrip2);

    flat2::FlatTest dst;
    FlatProto fp(dst);
    std::vector<std::pair<std::string, std::string>> ordered(flat.begin(), flat.end());
    sort_flat_keys_for_flatproto_apply(&ordered);
    for (const auto &kv : ordered) {
        ASSERT_TRUE(fp.set(kv.first, kv.second).ok()) << kv.first;
    }
    flat2::FlatInner out;
    ASSERT_TRUE(dst.detail().UnpackTo(&out));
    EXPECT_EQ(out.a(), 13);
}

}  // namespace
}  // namespace merak
