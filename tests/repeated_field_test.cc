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

#include <tests/proto/repeated.pb.h>
#include <gtest/gtest.h>
#include <merak/proto/pb_to_json.h>
#include <merak/proto/json_to_pb.h>
#include <merak/proto/descriptor.h>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>
#include <google/protobuf/util/message_differencer.h>
#include <google/protobuf/util/type_resolver.h>
#include <google/protobuf/util/type_resolver_util.h>
#include <turbo/utility/status.h>
#include <turbo/strings/escaping.h>
#include <turbo/strings/str_format.h>

class RepeatedFieldTest : public testing::Test {
protected:
    void SetUp() {}
    void TearDown() {}
};

TEST_F(RepeatedFieldTest, empty_array) {
    RepeatedMessage m;
    std::string json;

    ASSERT_TRUE(merak::proto_message_to_json(m, &json).ok());
    std::cout << json << std::endl;
    merak::json::Document doc;
    ASSERT_TRUE(merak::proto_message_to_json(m, doc).ok());
    std::cout << doc.to_string() << std::endl;

    m.add_strings();
    m.add_ints(1);
    m.add_msgs();
    json.clear();
    ASSERT_TRUE(merak::proto_message_to_json(m, &json).ok());
    std::cout << json << std::endl;
}

inline google::protobuf::util::TypeResolver *get_generated_type_resolver() {
    static std::unique_ptr<google::protobuf::util::TypeResolver> type_resolver;
    if (!type_resolver) {
        type_resolver.reset(google::protobuf::util::NewTypeResolverForDescriptorPool(
                /*url_prefix=*/"", google::protobuf::DescriptorPool::generated_pool()));
    }
    return type_resolver.get();
}

TEST_F(RepeatedFieldTest, any) {
    /*
    Mox m_string;
    m_string.mutable_detail()->set_value("abc");
    std::string json_string;

    ASSERT_TRUE(merak::proto_message_to_json(m_string, &json_string));
    std::cout << json_string << std::endl;

    const google::protobuf::Descriptor *descriptor = m_string.GetDescriptor();
    for (int i = 0; i < descriptor->field_count(); ++i) {
        std::cout<<descriptor->field(i)->type_name()<<std::endl;
        std::cout<<descriptor->field(i)->name()<<std::endl;
        std::cout<<descriptor->field(i)->cpp_type()<<std::endl;
        std::cout<<descriptor->field(i)->type()<<std::endl;
        std::cout<<descriptor->field(i)->message_type()<<std::endl;
        auto desc = descriptor->field(i)->message_type();
        std::cout<<std::string(30, '*')<<std::endl;
        for(int j = 0; j < desc->field_count(); ++j) {
            std::cout<<std::string(30, '#')<<std::endl;
            std::cout<<desc->field(j)->name()<<std::endl;
            std::cout<<desc->field(j)->cpp_type()<<std::endl;
            std::cout<<desc->field(j)->type_name()<<std::endl;
        }
    }
    auto ref = m_string.GetReflection();
*/
    auto checker = get_generated_type_resolver();
    google::protobuf::Type  t;
    auto rs = checker->ResolveMessageType("/Mox", &t);
    t.New();
    std::cout<<rs<<std::endl;
    std::cout<<std::string(30, '#')<<std::endl;
    if(rs.ok()) {
        std::cout << t.name() << std::endl;
        auto fd = t.GetDescriptor();
        std::cout<<"fields: "<<t.GetDescriptor()->field_count()<<std::endl;
        for(size_t i = 0; i < fd->field_count(); i++) {
            auto d = fd->field(i);
            std::cout<<std::string(30, '$')<<std::endl;
            std::cout <<"name: "<<d->name() << std::endl;
            std::cout <<"type: "<<d->type() << std::endl;
            std::cout<<"cpp_type: "<<d->cpp_type()<<std::endl;
            std::cout<<"is_repeated: "<<d->is_repeated()<<std::endl;
        }
    }
}

TEST_F(RepeatedFieldTest, des) {
    auto ptr = merak::create_message_by_type_name("M1");
    ASSERT_TRUE(ptr);

    Mox mm;
    mm.set_oabc(21);
    M1 m1;
    m1.set_a(10);
    M2 m2;
    m2.set_b("23");
    mm.mutable_detail()->PackFrom(m1);
    mm.mutable_details()->Add()->PackFrom(m1);
    mm.mutable_details()->Add()->PackFrom(m2);

    const google::protobuf::Descriptor *desc = mm.GetDescriptor();
    const google::protobuf::FieldDescriptor *detail_fd = desc->FindFieldByLowercaseName("detail");
    ASSERT_TRUE(detail_fd != nullptr);
    ASSERT_TRUE(merak::is_protobuf_any(detail_fd));
    const google::protobuf::FieldDescriptor *details_fd = desc->FindFieldByLowercaseName("details");
    ASSERT_TRUE(details_fd != nullptr);
    ASSERT_TRUE(merak::is_protobuf_any(details_fd));

    std::string o;
    ASSERT_TRUE(merak::proto_message_to_json(mm, &o).ok());
    merak::Pb2JsonOptions opt;
    opt.using_a_type_url = true;
    std::string oa;
    ASSERT_TRUE(merak::proto_message_to_json(mm, &oa, opt).ok());
    ASSERT_FALSE(o.empty());
    ASSERT_FALSE(oa.empty());

    std::string m1_b64;
    std::string m2_b64;
    turbo::base64_encode(m1.SerializeAsString(), &m1_b64);
    turbo::base64_encode(m2.SerializeAsString(), &m2_b64);
    std::string json = turbo::str_format(
            R"({"detail":{"type_url":"M1","value":"%s"},"details":[{"type_url":"M1","value":"%s"},{"type_url":"M2","value":"%s"}],"oAbc":21})",
            m1_b64,
            m1_b64,
            m2_b64);
    Mox mmm;
    ASSERT_TRUE(merak::json_to_proto_message(json, &mmm).ok());
    std::string json1 = turbo::str_format(
            R"({"detail":{"@type":"M1","value":"%s"},"details":[{"@type":"M1","value":"%s"},{"@type":"M2","value":"%s"}],"oAbc":21})",
            m1_b64,
            m1_b64,
            m2_b64);
    Mox mmm1;
    ASSERT_TRUE(merak::json_to_proto_message(json1, &mmm1).ok());

    EXPECT_TRUE(google::protobuf::util::MessageDifferencer::Equals(mm, mmm));
    EXPECT_TRUE(google::protobuf::util::MessageDifferencer::Equals(mm, mmm1));

    M1 m1_up;
    ASSERT_TRUE(mmm1.detail().UnpackTo(&m1_up));
    EXPECT_EQ(m1_up.a(), 10);

    merak::json::Document doc;
    ASSERT_TRUE(merak::proto_message_to_json(mmm1, doc).ok());
    ASSERT_FALSE(doc.to_string().empty());
}
