// Copyright (C) 2024 Kumo inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#include <sys/time.h>
#include <gtest/gtest.h>
#include <iostream>
#include <fstream>
#include <string>
#include <google/protobuf/text_format.h>
#include <merak/utility/streams.h>
#include <turbo/strings/str_format.h>
#include <merak/json.h>
#include <turbo/times/time.h>
#include <gperftools/profiler.h>
#include <merak/proto/pb_to_json.h>
#include <merak/proto/json_to_pb.h>
#include <merak/proto/encode_decode.h>
#include <merak/utility/zero_copy_stream_reader.h>
#include <turbo/times/time.h>
#include <tests/proto/message.pb.h>
#include <tests/proto/addressbook1.pb.h>
#include <tests/proto/addressbook.pb.h>
#include <tests/proto/addressbook_encode_decode.pb.h>
#include <tests/proto/addressbook_map.pb.h>
#include <tests/data/paths.h>

namespace {  // just for coding-style check

    using addressbook::AddressBook;
    using addressbook::Person;

    class ProtobufJsonTest : public testing::Test {
    protected:
        void SetUp() {}

        void TearDown() {}
    };

    inline int64_t gettimeofday_us() {
        timeval now;
        gettimeofday(&now, nullptr);
        return now.tv_sec * 1000000L + now.tv_usec;
    }

    TEST_F(ProtobufJsonTest, json_to_pb_normal_case) {
        const int N = 1000;
        int64_t total_tm = 0;
        int64_t total_tm2 = 0;
        for (int i = 0; i < N; ++i) {
            std::string info3 = "{\"content\":[{\"distance\":1,\"unknown_member\":2,\"ext\":"
                                "{\"age\":1666666666, \"databyte\":\"d2VsY29tZQ==\", \"enumtype\":1},"
                                "\"uid\":\"someone\"},{\"distance\":10,\"unknown_member\":20,"
                                "\"ext\":{\"age\":1666666660, \"databyte\":\"d2VsY29tZQ==\","
                                "\"enumtype\":2},\"uid\":\"someone0\"}], \"judge\":false,"
                                "\"spur\":2, \"data\":[1,2,3,4,5,6,7,8,9,10]}";

            JsonContextBody data;
            const int64_t tm1 = gettimeofday_us();
            const auto ret = merak::json_to_proto_message(info3, &data);
            const int64_t tm2 = gettimeofday_us();
            total_tm += tm2 - tm1;
            ASSERT_TRUE(ret.ok());

            std::string info4;
            const int64_t tm3 = gettimeofday_us();
            auto ret2 = merak::proto_message_to_json(data, &info4);
            const int64_t tm4 = gettimeofday_us();
            ASSERT_TRUE(ret2.ok());
            total_tm2 += tm4 - tm3;
            ASSERT_STREQ("{\"data\":[1,2,3,4,5,6,7,8,9,10],"
                         "\"judge\":false,\"spur\":2.0,\"content\":[{\"uid\":\"someone\","
                         "\"distance\":1.0,\"ext\":{\"age\":1666666666,\"databyte\":\"d2VsY29tZQ==\","
                         "\"enumtype\":\"HOME\"}},\{\"uid\":\"someone0\",\"distance\":10.0,\"ext\":"
                         "{\"age\":1666666660,\"databyte\":\"d2VsY29tZQ==\",\"enumtype\":\"WORK\"}}]}",
                         info4.data());
        }
        std::cout << "json2pb=" << total_tm / N
                  << "us pb2json=" << total_tm2 / N << "us"
                  << std::endl;
    }

    TEST_F(ProtobufJsonTest, json_base64_string_to_pb_types_case) {
        std::string info3 = "{\"content\":[{\"distance\":1,\"unknown_member\":2,\"ext\":"
                            "{\"age\":1666666666, \"databyte\":\"d2VsY29tZQ==\", \"enumtype\":1},"
                            "\"uid\":\"someone\"},{\"distance\":10,\"unknown_member\":20,"
                            "\"ext\":{\"age\":1666666660, \"databyte\":\"d2VsY29tZTA=\","
                            "\"enumtype\":2},\"uid\":\"someone0\"}], \"judge\":false,"
                            "\"spur\":2}";

        JsonContextBody data;
        merak::Json2PbOptions options_j2pb;
        options_j2pb.base64_to_bytes = true;
        const auto ret = merak::json_to_proto_message(info3, &data, options_j2pb);
        ASSERT_TRUE(ret.ok());
        ASSERT_TRUE(data.content_size() == 2);
        ASSERT_EQ(data.content(0).ext().databyte(), "welcome");
        ASSERT_EQ(data.content(1).ext().databyte(), "welcome0");

        std::string info4;
        merak::Pb2JsonOptions options_pb2j;
        options_pb2j.bytes_to_base64 = true;
        auto ret2 = merak::proto_message_to_json(data, &info4, options_pb2j);
        ASSERT_TRUE(ret2.ok());
        ASSERT_STREQ("{\"judge\":false,\"spur\":2.0,\"content\":[{\"uid\":\"someone\","
                     "\"distance\":1.0,\"ext\":{\"age\":1666666666,\"databyte\":\"d2VsY29tZQ==\","
                     "\"enumtype\":\"HOME\"}},\{\"uid\":\"someone0\",\"distance\":10.0,\"ext\":"
                     "{\"age\":1666666660,\"databyte\":\"d2VsY29tZTA=\",\"enumtype\":\"WORK\"}}]}",
                     info4.data());
    }

    TEST_F(ProtobufJsonTest, json_to_pb_map_case) {
        std::string json = "{\"addr\":\"baidu.com\","
                           "\"numbers\":{\"tel\":123456,\"cell\":654321},"
                           "\"contacts\":{\"email\":\"frank@baidu.com\","
                           "               \"office\":\"Shanghai\"},"
                           "\"friends\":{\"John\":[{\"school\":\"SJTU\",\"year\":2007}]}}";
        AddressNoMap ab1;
        ASSERT_TRUE(merak::json_to_proto_message(json, &ab1).ok());
        ASSERT_EQ("baidu.com", ab1.addr());

        AddressIntMap ab2;
        ASSERT_TRUE(merak::json_to_proto_message(json, &ab2).ok());
        ASSERT_EQ("baidu.com", ab2.addr());
        ASSERT_EQ("tel", ab2.numbers(0).key());
        ASSERT_EQ(123456, ab2.numbers(0).value());
        ASSERT_EQ("cell", ab2.numbers(1).key());
        ASSERT_EQ(654321, ab2.numbers(1).value());

        AddressStringMap ab3;
        ASSERT_TRUE(merak::json_to_proto_message(json, &ab3).ok());
        ASSERT_EQ("baidu.com", ab3.addr());
        ASSERT_EQ("email", ab3.contacts(0).key());
        ASSERT_EQ("frank@baidu.com", ab3.contacts(0).value());
        ASSERT_EQ("office", ab3.contacts(1).key());
        ASSERT_EQ("Shanghai", ab3.contacts(1).value());

        AddressComplex ab4;
        ASSERT_TRUE(merak::json_to_proto_message(json, &ab4).ok());
        ASSERT_EQ("baidu.com", ab4.addr());
        ASSERT_EQ("John", ab4.friends(0).key());
        ASSERT_EQ("SJTU", ab4.friends(0).value(0).school());
        ASSERT_EQ(2007, ab4.friends(0).value(0).year());

        std::string old_json = "{\"addr\":\"baidu.com\","
                               "\"numbers\":[{\"key\":\"tel\",\"value\":123456},"
                               "             {\"key\":\"cell\",\"value\":654321}]}";
        ab2.Clear();
        ASSERT_TRUE(merak::json_to_proto_message(old_json, &ab2).ok());
        ASSERT_EQ("baidu.com", ab2.addr());
        ASSERT_EQ("tel", ab2.numbers(0).key());
        ASSERT_EQ(123456, ab2.numbers(0).value());
        ASSERT_EQ("cell", ab2.numbers(1).key());
        ASSERT_EQ(654321, ab2.numbers(1).value());
    }

    TEST_F(ProtobufJsonTest, json_to_pb_encode_decode) {
        std::string info3 = "{\"@Content_Test%@\":[{\"Distance_info_\":1,\
                         \"_ext%T_\":{\"Aa_ge(\":1666666666, \"databyte(std::string)\":\
                         \"d2VsY29tZQ==\", \"enum--type\":\"HOME\"},\"uid*\":\"welcome\"}], \
                         \"judge\":false, \"spur\":2, \"data:array\":[]}";
        printf("----------test json to pb------------\n\n");
    
        JsonContextBodyEncDec data;
        ASSERT_TRUE(merak::json_to_proto_message(info3, &data).ok());

        std::string info4;
        ASSERT_TRUE(merak::proto_message_to_json(data, &info4).ok());
        ASSERT_STREQ("{\"judge\":false,\"spur\":2.0,"
                     "\"@Content_Test%@\":[{\"uid*\":\"welcome\",\"Distance_info_\":1.0,"
                     "\"_ext%T_\":{\"Aa_ge(\":1666666666,\"databyte(std::string)\":\"d2VsY29tZQ==\","
                     "\"enum--type\":\"HOME\"}}]}", info4.data());
    }

    TEST_F(ProtobufJsonTest, json_to_pb_unicode_case) {
        AddressBook address_book;

        Person *person = address_book.add_person();

        person->set_id(100);

        char name[255 * 1024];
        for (int j = 0; j < 255; j++) {
            for (int i = 0; i < 1024; i++) {
                name[j * 1024 + i] = i + 1;
            }
        }
        name[255 * 1024 - 1] = '\0';
        person->set_name(name);
        person->set_data(-240000000);
        person->set_data32(6);
        person->set_data64(-1820000000);
        person->set_datadouble(123.456);
        person->set_datadouble_scientific(1.23456789e+08);
        person->set_datafloat_scientific(1.23456789e+08);
        person->set_datafloat(8.6123);
        person->set_datau32(60);
        person->set_datau64(960);
        person->set_databool(0);
        person->set_databyte("welcome to china");
        person->set_datafix32(1);
        person->set_datafix64(666);
        person->set_datasfix32(120);
        person->set_datasfix64(-802);

        std::string info1;

        google::protobuf::TextFormat::Printer printer;
        std::string text;
        printer.PrintToString(*person, &text);

        printf("----------test pb to json------------\n\n");
        auto ret = merak::proto_message_to_json(address_book, &info1);
        ASSERT_TRUE(ret.ok());
        AddressBook address_book_test;
        ret = merak::json_to_proto_message(info1, &address_book_test);
        ASSERT_TRUE(ret.ok());
        std::string info2;
        ret = merak::proto_message_to_json(address_book_test, &info2);
        ASSERT_TRUE(ret.ok());
        ASSERT_TRUE(!info1.compare(info2));
        merak::CordOutputStream stream;
        auto res = merak::proto_message_to_json(address_book, &stream);
        turbo::Cord buf = stream.consume();
        ASSERT_TRUE(res.ok());
        merak::CordInputStream stream2(&buf);
        AddressBook address_book_test3;
        ret = merak::json_to_proto_message(&stream2, &address_book_test3);
        ASSERT_TRUE(ret.ok());
        std::string info3;
        ret = merak::proto_message_to_json(address_book_test3, &info3);
        ASSERT_TRUE(ret.ok());
        ASSERT_TRUE(!info2.compare(info3));
    }

    TEST_F(ProtobufJsonTest, json_to_pb_edge_case) {
        std::string info3 = "{\"judge\":false, \"spur\":2.0e1}";
        
        JsonContextBody data;
        auto ret = merak::json_to_proto_message(info3, &data);
        ASSERT_TRUE(ret.ok());

        std::string info4;
        ret = merak::proto_message_to_json(data, &info4);
        ASSERT_TRUE(ret.ok());

        info3 = "{\"judge\":false, \"spur\":-2, \"data\":[], \"info\":[],\"content\":[]}";

        JsonContextBody data1;
        ret = merak::json_to_proto_message(info3, &data1);
        ASSERT_TRUE(ret.ok());

        info4.clear();
        ret = merak::proto_message_to_json(data1, &info4);
        ASSERT_TRUE(ret.ok());

        info3 = "{\"judge\":false, \"spur\":\"NaN\"}";
        
        JsonContextBody data2;
        ret = merak::json_to_proto_message(info3, &data2);
        ASSERT_TRUE(ret.ok());

        info4.clear();
        ret = merak::proto_message_to_json(data2, &info4);
        ASSERT_TRUE(ret.ok());

        info3 = "{\"judge\":false, \"spur\":\"Infinity\"}";

        JsonContextBody data3;
        ret = merak::json_to_proto_message(info3, &data3);
        ASSERT_TRUE(ret.ok());

        info4.clear();
        ret = merak::proto_message_to_json(data3, &info4);
        ASSERT_TRUE(ret.ok());

        info3 = "{\"judge\":false, \"spur\":\"-inFiNITY\"}";

        JsonContextBody data4;
        ret = merak::json_to_proto_message(info3, &data4);
        ASSERT_TRUE(ret.ok());

        info4.clear();
        ret = merak::proto_message_to_json(data4, &info4);
        ASSERT_TRUE(ret.ok());

        info3 = "{\"judge\":false, \"spur\":2.0, \"content\":[{\"distance\":2.5, "
                "\"ext\":{\"databyte\":\"d2VsY29tZQ==\", \"enumtype\":\"MOBILE\"}}]}";

        JsonContextBody data5;
        ret = merak::json_to_proto_message(info3, &data5);
        ASSERT_TRUE(ret.ok());

        info4.clear();
        ret = merak::proto_message_to_json(data5, &info4);
        ASSERT_TRUE(ret.ok());

        info3 = "{\"content\":[{\"distance\":1,\"unknown_member\":2,\
              \"ext\":{\"age\":1666666666, \"databyte\":\"d2VsY29tZQ==\", \"enumtype\":1},\
              \"uid\":\"someone\"},{\"distance\":2.3,\"unknown_member\":20,\
              \"ext\":{\"age\":1666666660, \"databyte\":\"d2VsY29tZQ==\", \"enumtype\":\"Test\"},\
              \"uid\":\"someone0\"}], \"judge\":false, \
              \"spur\":2, \"data\":[1,2,3,4,5,6,7,8,9,10]}";

        JsonContextBody data9;
        ret = merak::json_to_proto_message(info3, &data9);
        ASSERT_TRUE(ret.ok());
        //auto err= ret.get_kv_payload("optional");
        //std::cout<<"payload: "<<err.has_value()<<std::endl;
        //ASSERT_STREQ("Invalid value `\"Test\"' for optional field `Ext.enumtype' which SHOULD be enum", ret.get_kv_payload("optonal")->c_str());

        info3 = "{\"content\":[{\"distance\":1,\"unknown_member\":2,\
              \"ext\":{\"age\":1666666666, \"databyte\":\"d2VsY29tZQ==\", \"enumtype\":1},\
              \"uid\":\"someone\"},{\"distance\":5,\"unknown_member\":20,\
              \"ext\":{\"age\":1666666660, \"databyte\":\"d2VsY29tZQ==\", \"enumtype\":15},\
              \"uid\":\"someone0\"}], \"judge\":false, \
              \"spur\":2, \"data\":[1,2,3,4,5,6,7,8,9,10]}";
        
        JsonContextBody data10;
        ret = merak::json_to_proto_message(info3, &data10);
        ASSERT_TRUE(ret.ok());
        //ASSERT_STREQ("Invalid value `15' for optional field `Ext.enumtype' which SHOULD be enum", ret.get_kv_payload("optional")->c_str());

        info3 = "{\"content\":[{\"distance\":1,\"unknown_member\":2,\
              \"ext\":{\"age\":1666666666, \"databyte\":\"d2VsY29tZQ==\", \"enumtype\":1},\
              \"uid\":\"someone\"},{\"distance\":5,\"unknown_member\":20,\
              \"ext\":{\"age\":1666666660, \"databyte\":\"d2VsY29tZQ==\", \"enumtype\":15},\
              \"uid\":\"someone0\"}], \"judge\":false, \
              \"spur\":2, \"type\":[\"123\"]}";
        
        JsonContextBody data11;
        ret = merak::json_to_proto_message(info3, &data11);
        ASSERT_TRUE(ret.ok());
       // ASSERT_STREQ(
       //         "Invalid value `array' for optional field `JsonContextBody.type' which SHOULD be INT64, Invalid value `15' for optional field `Ext.enumtype' which SHOULD be enum",
       //         ret.get_kv_payload("optional")->c_str());
    }

    TEST_F(ProtobufJsonTest, json_to_pb_expected_failed_case) {
        std::string info3 = "{\"content\":[{\"unknown_member\":2,\"ext\":{\"age\":1666666666, \
                          \"databyte\":\"welcome\", \"enumtype\":1},\"uid\":\"someone\"},\
                          {\"unknown_member\":20,\"ext\":{\"age\":1666666660, \"databyte\":\
                          \"welcome0\", \"enumtype\":2},\"uid\":\"someone0\"}], \
                          \"judge\":false, \"spur\":2, \"data\":[1,2,3,4,5,6,7,8,9,10]}";

        JsonContextBody data;
        auto ret = merak::json_to_proto_message(info3, &data);
        ASSERT_FALSE(ret.ok());
        ASSERT_STREQ("Missing required field: Content.distance", ret.message().c_str());

        info3 = "{\"content\":[{\"distance\":1,\"unknown_member\":2,\"ext\":{\"age\":1666666666, \
              \"enumtype\":1},\"uid\":\"someone\"},{\"distance\":10,\"unknown_member\":20,\
              \"ext\":{\"age\":1666666660, \"databyte\":\"welcome0\", \"enumtype\":2},\
              \"uid\":\"someone0\"}], \"judge\":false, \
              \"spur\":2, \"data\":[1,2,3,4,5,6,7,8,9,10]}";
       

        JsonContextBody data2;
        ret = merak::json_to_proto_message(info3, &data2);
        ASSERT_FALSE(ret.ok());
        ASSERT_STREQ("Missing required field: Ext.databyte", ret.message().c_str());

        info3 = "{\"content\":[{\"distance\":1,\"unknown_member\":2,\
              \"ext\":{\"age\":1666666666, \"databyte\":\"welcome\", \"enumtype\":1},\
              \"uid\":\"someone\"},{\"distance\":10,\"unknown_member\":20,\
              \"ext\":{\"age\":1666666660, \"databyte\":\"welcome0\", \"enumtype\":2},\
              \"uid\":\"someone0\"}], \"spur\":2, \"data\":[1,2,3,4,5,6,7,8,9,10]}";

        JsonContextBody data3;
        ret = merak::json_to_proto_message(info3, &data);
        ASSERT_FALSE(ret.ok());
        ASSERT_STREQ("Missing required field: JsonContextBody.judge", ret.message().c_str());

        info3 = "{\"content\":[{\"distance\":1,\"unknown_member\":2,\
              \"ext\":{\"age\":1666666666, \"databyte\":\"welcome\", \"enumtype\":1},\
              \"uid\":\"someone\"},{\"distance\":10,\"unknown_member\":20,\
              \"ext\":{\"age\":1666666660, \"databyte\":\"welcome0\", \"enumtype\":2},\
              \"uid\":\"someone0\"}], \"judge\":\"false\", \
              \"spur\":2, \"data\":[1,2,3,4,5,6,7,8,9,10]}";
    

        JsonContextBody data4;
        ret = merak::json_to_proto_message(info3, &data4);
        ASSERT_FALSE(ret.ok());
        ASSERT_STREQ("Invalid value `\"false\"' for field `JsonContextBody.judge' which SHOULD be BOOL", ret.message().c_str());

        info3 = "{\"content\":[{\"distance\":1,\"unknown_member\":2,\
              \"ext\":{\"age\":1666666666, \"databyte\":\"welcome\", \"enumtype\":1},\
              \"uid\":\"someone\"},{\"distance\":10,\"unknown_member\":20,\
              \"ext\":{\"age\":1666666660, \"databyte\":\"welcome0\", \"enumtype\":2},\
              \"uid\":\"someone0\"}], \"judge\":false, \
              \"spur\":2, \"data\":[\"1\",\"2\",\"3\",\"4\"]}";
        

        JsonContextBody data5;
        ret = merak::json_to_proto_message(info3, &data5);
        ASSERT_FALSE(ret.ok());
        ASSERT_STREQ("Invalid value `\"1\"' for field `JsonContextBody.data' which SHOULD be INT32", ret.message().c_str());

        info3 = "{\"content\":[{\"distance\":1,\"unknown_member\":2,\
              \"ext\":{\"age\":1666666666, \"databyte\":\"welcome\", \"enumtype\":1},\
              \"uid\":\"someone\"},{\"distance\":10,\"unknown_member\":20,\
              \"ext\":{\"age\":1666666660, \"databyte\":\"welcome0\", \"enumtype\":2},\
              \"uid\":\"someone0\"}], \"judge\":false, \
              \"spur\":2, \"data\":[1,2,3,4,5,6,7,8,9,10], \"info\":2}";
        

        JsonContextBody data6;
        ret = merak::json_to_proto_message(info3, &data6);
        ASSERT_FALSE(ret.ok());
        ASSERT_STREQ("Invalid value for repeated field: JsonContextBody.info", ret.message().c_str());

        info3 = "{\"judge\":false, \"spur\":\"NaNa\"}";
        

        JsonContextBody data7;
        ret = merak::json_to_proto_message(info3, &data7);
        ASSERT_FALSE(ret.ok());
        ASSERT_STREQ("Invalid value `\"NaNa\"' for field `JsonContextBody.spur' which SHOULD be d",
                     ret.message().c_str());

        info3 = "{\"judge\":false, \"spur\":\"Infinty\"}";
        

        JsonContextBody data8;
        ret = merak::json_to_proto_message(info3, &data8);
        ASSERT_FALSE(ret.ok());
        ASSERT_STREQ("Invalid value `\"Infinty\"' for field `JsonContextBody.spur' which SHOULD be d",
                     ret.message().c_str());

        info3 = "{\"content\":[{\"distance\":1,\"unknown_member\":2,\"ext\":{\"age\":1666666666, \
              \"enumtype\":1},\"uid\":23},{\"distance\":10,\"unknown_member\":20,\
              \"ext\":{\"age\":1666666660, \"databyte\":\"welcome0\", \"enumtype\":2},\
              \"uid\":\"someone0\"}], \"judge\":false, \
              \"spur\":2, \"data\":[1,2,3,4,5,6,7,8,9,10]}";
        

        JsonContextBody data9;
        ret = merak::json_to_proto_message(info3, &data9);
        ASSERT_FALSE(ret.ok());
        ASSERT_STREQ(
                "Invalid value `23' for optional field `Content.uid' which SHOULD be string, Missing required field: Ext.databyte",
                ret.message().c_str());
    }

    TEST_F(ProtobufJsonTest, json_to_pb_perf_case) {

        std::string info3 = "{\"content\":[{\"distance\":1.0,\
                          \"ext\":{\"age\":1666666666, \"databyte\":\"d2VsY29tZQ==\", \"enumtype\":1},\
                          \"uid\":\"welcome\"}], \"judge\":false, \"spur\":2.0, \"data\":[]}";

        printf("----------test json to pb performance------------\n\n");

        std::string error;

        ProfilerStart("json_to_pb_perf.prof");
        turbo::TimeCost timer;
        turbo::Status res;
        float avg_time1 = 0;
        float avg_time2 = 0;
        const int times = 100000;
        for (int i = 0; i < times; i++) {
            JsonContextBody data;
            timer.reset();
            res = merak::json_to_proto_message(info3, &data);
            timer.stop();
            avg_time1 += timer.u_elapsed();
            ASSERT_TRUE(res.ok());

            std::string info4;
            timer.reset();
            res = merak::proto_message_to_json(data, &info4);
            timer.stop();
            avg_time2 += timer.u_elapsed();
            ASSERT_TRUE(res.ok());
        }
        avg_time1 /= times;
        avg_time2 /= times;
        ProfilerStop();
        printf("avg time to convert json to pb is %fus\n", avg_time1);
        printf("avg time to convert pb to json is %fus\n", avg_time2);
    }

    TEST_F(ProtobufJsonTest, json_to_pb_encode_decode_perf_case) {
        std::string info3 = "{\"@Content_Test%@\":[{\"Distance_info_\":1,\
                          \"_ext%T_\":{\"Aa_ge(\":1666666666, \"databyte(std::string)\":\
                          \"welcome\", \"enum--type\":1},\"uid*\":\"welcome\"}], \
                          \"judge\":false, \"spur\":2, \"data:array\":[]}";
        printf("----------test json to pb encode/decode performance------------\n\n");
        std::string error;

        ProfilerStart("json_to_pb_encode_decode_perf.prof");
        turbo::TimeCost timer;
        turbo::Status res;
        float avg_time1 = 0;
        float avg_time2 = 0;
        const int times = 100000;
        for (int i = 0; i < times; i++) {
            JsonContextBody data;
            timer.reset();
            res = merak::json_to_proto_message(info3, &data);
            timer.stop();
            avg_time1 += timer.u_elapsed();
            ASSERT_TRUE(res.ok());

            std::string info4;
            timer.reset();
            res = merak::proto_message_to_json(data, &info4);
            timer.stop();
            avg_time2 += timer.u_elapsed();
            ASSERT_TRUE(res.ok());
        }
        avg_time1 /= times;
        avg_time2 /= times;
        ProfilerStop();
        printf("avg time to convert json to pb is %fus\n", avg_time1);
        printf("avg time to convert pb to json is %fus\n", avg_time2);
    }

    TEST_F(ProtobufJsonTest, json_to_pb_complex_perf_case) {
        std::ifstream in(merak::test_jsonout, std::ios::in);
        std::ostringstream tmp;
        tmp << in.rdbuf();
        turbo::Cord buf;
        buf.append(tmp.str());
        in.close();

        printf("----------test json to pb performance------------\n\n");

        std::string error;

        turbo::TimeCost timer;

        turbo::Status res;
        float avg_time1 = 0;
        const int times = 10000;
        merak::Json2PbOptions options;
        options.base64_to_bytes = false;
        ProfilerStart("json_to_pb_complex_perf.prof");
        for (int i = 0; i < times; i++) {
            gss::message::gss_us_res_t data;
            merak::CordInputStream stream(&buf);
            timer.reset();
            res = merak::json_to_proto_message(&stream, &data, options);
            timer.stop();
            avg_time1 += timer.u_elapsed();
            ASSERT_TRUE(res.ok());
        }
        ProfilerStop();
        avg_time1 /= times;
        printf("avg time to convert json to pb is %fus\n", avg_time1);
    }

    TEST_F(ProtobufJsonTest, json_to_pb_to_string_complex_perf_case) {
        std::ifstream in(merak::test_jsonout, std::ios::in);
        std::ostringstream tmp;
        tmp << in.rdbuf();
        std::string info3 = tmp.str();
        in.close();

        printf("----------test json to pb performance------------\n\n");

        std::string error;

        turbo::TimeCost timer;
        turbo::Status res;
        float avg_time1 = 0;
        const int times = 10000;
        merak::Json2PbOptions options;
        options.base64_to_bytes = false;
        ProfilerStart("json_to_pb_to_string_complex_perf.prof");
        for (int i = 0; i < times; i++) {
            gss::message::gss_us_res_t data;
            timer.reset();
            res = merak::json_to_proto_message(info3, &data, options);
            timer.stop();
            avg_time1 += timer.u_elapsed();
            ASSERT_TRUE(res.ok());
        }
        avg_time1 /= times;
        ProfilerStop();
        printf("avg time to convert json to pb is %fus\n", avg_time1);
    }

    TEST_F(ProtobufJsonTest, pb_to_json_normal_case) {
        AddressBook address_book;

        Person *person = address_book.add_person();

        person->set_id(100);
        person->set_name("baidu");
        person->set_email("welcome@baidu.com");

        Person::PhoneNumber *phone_number = person->add_phone();
        phone_number->set_number("number123");
        phone_number->set_type(Person::HOME);

        person->set_data(-240000000);
        person->set_data32(6);
        person->set_data64(-1820000000);
        person->set_datadouble(123.456);
        person->set_datadouble_scientific(1.23456789e+08);
        person->set_datafloat_scientific(1.23456789e+08);
        person->set_datafloat(8.6123);
        person->set_datau32(60);
        person->set_datau64(960);
        person->set_databool(0);
        person->set_databyte("welcome");
        person->set_datafix32(1);
        person->set_datafix64(666);
        person->set_datasfix32(120);
        person->set_datasfix64(-802);

        std::string info1;

        google::protobuf::TextFormat::Printer printer;
        std::string text;
        printer.PrintToString(*person, &text);

        printf("text:%s\n", text.data());

        printf("----------test pb to json------------\n\n");
        merak::Pb2JsonOptions option;
        option.bytes_to_base64 = true;
        auto ret = merak::proto_message_to_json(address_book, &info1, option);
        ASSERT_TRUE(ret.ok());

        ASSERT_STREQ("{\"person\":[{\"name\":\"baidu\",\"id\":100,\"email\":\"welcome@baidu.com\","
                     "\"phone\":[{\"number\":\"number123\",\"type\":\"HOME\"}],\"data\":-240000000,"
                     "\"data32\":6,\"data64\":-1820000000,\"datadouble\":123.456,"
                     "\"datafloat\":8.612299919128418,\"datau32\":60,\"datau64\":960,"
                     "\"databool\":false,\"databyte\":\"d2VsY29tZQ==\",\"datafix32\":1,"
                     "\"datafix64\":666,\"datasfix32\":120,\"datasfix64\":-802,"
                     "\"datafloat_scientific\":123456792.0,\"datadouble_scientific\":123456789.0}]}",
                     info1.data());

        info1.clear();
        {
            merak::Pb2JsonOptions option;
            option.bytes_to_base64 = true;
            ret = proto_message_to_json(address_book, &info1, option);
        }
        ASSERT_TRUE(ret.ok());

        ASSERT_STREQ("{\"person\":[{\"name\":\"baidu\",\"id\":100,\"email\":\"welcome@baidu.com\","
                     "\"phone\":[{\"number\":\"number123\",\"type\":\"HOME\"}],\"data\":-240000000,"
                     "\"data32\":6,\"data64\":-1820000000,\"datadouble\":123.456,"
                     "\"datafloat\":8.612299919128418,\"datau32\":60,\"datau64\":960,"
                     "\"databool\":false,\"databyte\":\"d2VsY29tZQ==\",\"datafix32\":1,"
                     "\"datafix64\":666,\"datasfix32\":120,\"datasfix64\":-802,"
                     "\"datafloat_scientific\":123456792.0,\"datadouble_scientific\":123456789.0}]}",
                     info1.data());


        info1.clear();
        {
            merak::Pb2JsonOptions option;
            option.bytes_to_base64 = true;
            option.enum_option = merak::OUTPUT_ENUM_BY_NUMBER;
            ret = proto_message_to_json(address_book, &info1, option);
        }
        ASSERT_TRUE(ret.ok());

        ASSERT_STREQ("{\"person\":[{\"name\":\"baidu\",\"id\":100,\"email\":\"welcome@baidu.com\","
                     "\"phone\":[{\"number\":\"number123\",\"type\":1}],\"data\":-240000000,"
                     "\"data32\":6,\"data64\":-1820000000,\"datadouble\":123.456,"
                     "\"datafloat\":8.612299919128418,\"datau32\":60,\"datau64\":960,"
                     "\"databool\":false,\"databyte\":\"d2VsY29tZQ==\",\"datafix32\":1,"
                     "\"datafix64\":666,\"datasfix32\":120,\"datasfix64\":-802,"
                     "\"datafloat_scientific\":123456792.0,\"datadouble_scientific\":123456789.0}]}",
                     info1.data());

        printf("----------test json to pb------------\n\n");

        const int N = 1000;
        int64_t total_tm = 0;
        int64_t total_tm2 = 0;
        for (int i = 0; i < N; ++i) {

            std::string info3;
            AddressBook data1;
            std::string error1;
            const int64_t tm1 = gettimeofday_us();
            auto ret1 = merak::json_to_proto_message(info1, &data1);
            const int64_t tm2 = gettimeofday_us();
            total_tm += tm2 - tm1;
            ASSERT_TRUE(ret1.ok());

            std::string error2;
            const int64_t tm3 = gettimeofday_us();
            ret1 = merak::proto_message_to_json(data1, &info3);
            const int64_t tm4 = gettimeofday_us();
            ASSERT_TRUE(ret1.ok());
            total_tm2 += tm4 - tm3;

            ASSERT_STREQ("{\"person\":[{\"name\":\"baidu\",\"id\":100,\"email\":\"welcome@baidu.com\","
                         "\"phone\":[{\"number\":\"number123\",\"type\":\"HOME\"}],\"data\":-240000000,"
                         "\"data32\":6,\"data64\":-1820000000,\"datadouble\":123.456,"
                         "\"datafloat\":8.612299919128418,\"datau32\":60,\"datau64\":960,"
                         "\"databool\":false,\"databyte\":\"d2VsY29tZQ==\",\"datafix32\":1,"
                         "\"datafix64\":666,\"datasfix32\":120,\"datasfix64\":-802,"
                         "\"datafloat_scientific\":123456792.0,\"datadouble_scientific\":123456789.0}]}",
                         info3.data());

        }
        std::cout << "json2pb=" << total_tm / N
                  << "us pb2json=" << total_tm2 / N << "us"
                  << std::endl;
    }

    TEST_F(ProtobufJsonTest, pb_to_json_map_case) {
        std::string json = "{\"addr\":\"baidu.com\","
                           "\"numbers\":{\"tel\":123456,\"cell\":654321},"
                           "\"contacts\":{\"email\":\"frank@baidu.com\","
                           "               \"office\":\"Shanghai\"},"
                           "\"friends\":{\"John\":[{\"school\":\"SJTU\",\"year\":2007}]}}";
        std::string output;
        std::string error;
        AddressNoMap ab1;
        ASSERT_TRUE(merak::json_to_proto_message(json, &ab1).ok());
        ASSERT_TRUE(merak::proto_message_to_json(ab1, &output).ok());
        ASSERT_TRUE(output.find("\"addr\":\"baidu.com\"") != std::string::npos);

        output.clear();
        AddressIntMap ab2;
        ASSERT_TRUE(merak::json_to_proto_message(json, &ab2).ok());
        ASSERT_TRUE(merak::proto_message_to_json(ab2, &output).ok());
        ASSERT_TRUE(output.find("\"addr\":\"baidu.com\"") != std::string::npos);
        ASSERT_TRUE(output.find("\"tel\":123456") != std::string::npos);
        ASSERT_TRUE(output.find("\"cell\":654321") != std::string::npos);

        output.clear();
        AddressStringMap ab3;
        ASSERT_TRUE(merak::json_to_proto_message(json, &ab3).ok());
        ASSERT_TRUE(merak::proto_message_to_json(ab3, &output).ok());
        ASSERT_TRUE(output.find("\"addr\":\"baidu.com\"") != std::string::npos);
        ASSERT_TRUE(output.find("\"email\":\"frank@baidu.com\"") != std::string::npos);
        ASSERT_TRUE(output.find("\"office\":\"Shanghai\"") != std::string::npos);

        output.clear();
        AddressComplex ab4;
        ASSERT_TRUE(merak::json_to_proto_message(json, &ab4).ok());
        ASSERT_TRUE(merak::proto_message_to_json(ab4, &output).ok());
        ASSERT_TRUE(output.find("\"addr\":\"baidu.com\"") != std::string::npos);
        ASSERT_TRUE(output.find("\"friends\":{\"John\":[{\"school\":\"SJTU\","
                                "\"year\":2007}]}") != std::string::npos);
    }

    TEST_F(ProtobufJsonTest, pb_to_json_encode_decode) {
        JsonContextBodyEncDec json_data;
        json_data.set_type(80000);
        json_data.add_data_z058_array(200);
        json_data.add_data_z058_array(300);
        json_data.add_info("this is json data's info");
        json_data.add_info("this is a test");
        json_data.set_judge(true);
        json_data.set_spur(3.45);

        ContentEncDec *content = json_data.add__z064_content_test_z037__z064_();
        content->set_uid_z042_("content info");
        content->set_distance_info_(1234.56);

        ExtEncDec *ext = content->mutable__ext_z037_t_();
        ext->set_aa_ge_z040_(160000);
        ext->set_databyte_z040_std_z058__z058_string_z041_("databyte");
        ext->set_enum_z045__z045_type(ExtEncDec_PhoneTypeEncDec_WORK);

        std::string info1;

        google::protobuf::TextFormat::Printer printer;
        std::string text;
        printer.PrintToString(json_data, &text);

        printf("text:%s\n", text.data());

        printf("----------test pb to json------------\n\n");
        merak::Pb2JsonOptions option;
        option.bytes_to_base64 = true;
        ASSERT_TRUE(proto_message_to_json(json_data, &info1, option).ok());

        ASSERT_STREQ("{\"info\":[\"this is json data's info\",\"this is a test\"],\"type\":80000,"
                     "\"data:array\":[200,300],\"judge\":true,\"spur\":3.45,\"@Content_Test%@\":"
                     "[{\"uid*\":\"content info\",\"Distance_info_\":1234.56005859375,\"_ext%T_\":"
                     "{\"Aa_ge(\":160000,\"databyte(std::string)\":\"ZGF0YWJ5dGU=\","
                     "\"enum--type\":\"WORK\"}}]}",
                     info1.data());

        printf("----------test json to pb------------\n\n");

        std::string info3;
        JsonContextBodyEncDec data1;
        ASSERT_TRUE(merak::json_to_proto_message(info1, &data1).ok());
        ASSERT_TRUE(merak::proto_message_to_json(data1, &info3).ok());
        ASSERT_STREQ(info1.data(), info3.data());

        printf("----------test single repeated pb to json array------------\n\n");

        AddressBookEncDec single_repeated_json_data;

        auto person = single_repeated_json_data.add_person();
        person->set_id(1);
        person->set_name("foo");
        *person->mutable_json_body() = json_data;
        option.bytes_to_base64 = true;

        std::string text1;
        printer.PrintToString(single_repeated_json_data, &text1);

        printf("text1:\n\n%s\n", text1.data());

        std::string info4;
        option.single_repeated_to_array = false;
        ASSERT_TRUE(proto_message_to_json(single_repeated_json_data, &info4, option).ok());
        ASSERT_STREQ("{\"person\":["
                     "{\"name\":\"foo\",\"id\":1,\"json_body\":"
                     "{\"info\":[\"this is json data's info\",\"this is a test\"],\"type\":80000,"
                     "\"data:array\":[200,300],\"judge\":true,\"spur\":3.45,\"@Content_Test%@\":"
                     "[{\"uid*\":\"content info\",\"Distance_info_\":1234.56005859375,\"_ext%T_\":"
                     "{\"Aa_ge(\":160000,\"databyte(std::string)\":\"ZGF0YWJ5dGU=\","
                     "\"enum--type\":\"WORK\"}}]}}]}",
                     info4.data());


        std::string info5;
        option.single_repeated_to_array = true;
        ASSERT_TRUE(proto_message_to_json(single_repeated_json_data, &info5, option).ok());
        ASSERT_STREQ("[{\"name\":\"foo\",\"id\":1,\"json_body\":"
                     "{\"info\":[\"this is json data's info\",\"this is a test\"],\"type\":80000,"
                     "\"data:array\":[200,300],\"judge\":true,\"spur\":3.45,\"@Content_Test%@\":"
                     "[{\"uid*\":\"content info\",\"Distance_info_\":1234.56005859375,\"_ext%T_\":"
                     "{\"Aa_ge(\":160000,\"databyte(std::string)\":\"ZGF0YWJ5dGU=\","
                     "\"enum--type\":\"WORK\"}}]}}]",
                     info5.data());


        printf("----------test json array to single repeated pb------------\n\n");

        std::string info6;
        AddressBookEncDec data2;
        // object -> pb
        ASSERT_TRUE(merak::json_to_proto_message(info4, &data2).ok());
        ASSERT_TRUE(merak::proto_message_to_json(data2, &info6, option).ok());

        ASSERT_STREQ(info6.data(), info5.data());

        std::string info7;
        AddressBookEncDec data3;
        merak::Json2PbOptions option2;
        option2.array_to_single_repeated = true;
        // array -> pb
        ASSERT_TRUE(merak::json_to_proto_message(info5, &data3, option2).ok());
        ASSERT_TRUE(merak::proto_message_to_json(data3, &info7, option).ok());
        ASSERT_STREQ(info7.data(), info5.data());

        std::string info8;
        option.single_repeated_to_array = false;
        ASSERT_TRUE(merak::proto_message_to_json(data3, &info8, option).ok());
        ASSERT_STREQ(info8.data(), info4.data());
    }

    TEST_F(ProtobufJsonTest, pb_to_json_control_char_case) {
        AddressBook address_book;

        Person *person = address_book.add_person();

        person->set_id(100);
        char ch = 0x01;
        char *name = new char[17];
        memcpy(name, "baidu ", 6);
        name[6] = ch;
        char c = 0x08;
        char t = 0x1A;
        memcpy(name + 7, "test", 4);
        name[11] = c;
        name[12] = t;
        memcpy(name + 13, "end", 3);
        name[16] = '\0';
        person->set_name(name);
        printf("name is %s\n", name);
        person->set_email("welcome@baidu.com");

        Person::PhoneNumber *phone_number = person->add_phone();
        phone_number->set_number("number123");
        phone_number->set_type(Person::HOME);

        person->set_data(-240000000);
        person->set_data32(6);
        person->set_data64(-1820000000);
        person->set_datadouble(123.456);
        person->set_datadouble_scientific(1.23456789e+08);
        person->set_datafloat_scientific(1.23456789e+08);
        person->set_datafloat(8.6123);
        person->set_datau32(60);
        person->set_datau64(960);
        person->set_databool(0);
        person->set_databyte("welcome to china");
        person->set_datafix32(1);
        person->set_datafix64(666);
        person->set_datasfix32(120);
        person->set_datasfix64(-802);

        std::string info1;

        google::protobuf::TextFormat::Printer printer;
        std::string text;
        printer.PrintToString(*person, &text);

        printf("text:%s\n", text.data());

        turbo::Status ret;
        printf("----------test pb to json------------\n\n");
        {
            merak::Pb2JsonOptions option;
            option.bytes_to_base64 = false;
            ret = proto_message_to_json(address_book, &info1, option);
            ASSERT_TRUE(ret.ok());
        }


        ASSERT_STREQ("{\"person\":[{\"name\":\"baidu \\u0001test\\b\\u001Aend\",\"id\":100,\"email\":"
                     "\"welcome@baidu.com\","
                     "\"phone\":[{\"number\":\"number123\",\"type\":\"HOME\"}],\"data\":-240000000,"
                     "\"data32\":6,\"data64\":-1820000000,\"datadouble\":123.456,"
                     "\"datafloat\":8.612299919128418,\"datau32\":60,\"datau64\":960,"
                     "\"databool\":false,\"databyte\":\"welcome to china\",\"datafix32\":1,"
                     "\"datafix64\":666,\"datasfix32\":120,\"datasfix64\":-802,"
                     "\"datafloat_scientific\":123456792.0,\"datadouble_scientific\":123456789.0}]}",
                     info1.data());


        info1.clear();
        {
            merak::Pb2JsonOptions option;
            option.bytes_to_base64 = true;
            ret = proto_message_to_json(address_book, &info1, option);
            ASSERT_TRUE(ret.ok());
        }

        ASSERT_STREQ("{\"person\":[{\"name\":\"baidu \\u0001test\\b\\u001Aend\",\"id\":100,\"email\":"
                     "\"welcome@baidu.com\","
                     "\"phone\":[{\"number\":\"number123\",\"type\":\"HOME\"}],\"data\":-240000000,"
                     "\"data32\":6,\"data64\":-1820000000,\"datadouble\":123.456,"
                     "\"datafloat\":8.612299919128418,\"datau32\":60,\"datau64\":960,"
                     "\"databool\":false,\"databyte\":\"d2VsY29tZSB0byBjaGluYQ==\",\"datafix32\":1,"
                     "\"datafix64\":666,\"datasfix32\":120,\"datasfix64\":-802,"
                     "\"datafloat_scientific\":123456792.0,\"datadouble_scientific\":123456789.0}]}",
                     info1.data());


        info1.clear();
        {
            merak::Pb2JsonOptions option;
            option.enum_option = merak::OUTPUT_ENUM_BY_NUMBER;
            option.bytes_to_base64 = false;
            ret = proto_message_to_json(address_book, &info1, option);
            ASSERT_TRUE(ret.ok());
        }

        ASSERT_STREQ("{\"person\":[{\"name\":\"baidu \\u0001test\\b\\u001Aend\",\"id\":100,\"email\":"
                     "\"welcome@baidu.com\","
                     "\"phone\":[{\"number\":\"number123\",\"type\":1}],\"data\":-240000000,"
                     "\"data32\":6,\"data64\":-1820000000,\"datadouble\":123.456,"
                     "\"datafloat\":8.612299919128418,\"datau32\":60,\"datau64\":960,"
                     "\"databool\":false,\"databyte\":\"welcome to china\",\"datafix32\":1,"
                     "\"datafix64\":666,\"datasfix32\":120,\"datasfix64\":-802,"
                     "\"datafloat_scientific\":123456792.0,\"datadouble_scientific\":123456789.0}]}",
                     info1.data());

    }

    TEST_F(ProtobufJsonTest, json_to_pb_compatible_case) {
        AddressBook address_book;

        Person *person = address_book.add_person();

        person->set_id(100);
        char ch = 0x01;
        char *name = new char[17];
        memcpy(name, "baidu ", 6);
        name[6] = ch;
        char c = 0x08;
        char t = 0x1A;
        memcpy(name + 7, "test", 4);
        name[11] = c;
        name[12] = t;
        memcpy(name + 13, "end", 3);
        name[16] = '\0';
        person->set_name(name);
        printf("name is %s\n", name);
        person->set_email("welcome@baidu.com");

        Person::PhoneNumber *phone_number = person->add_phone();
        phone_number->set_number("number123");
        phone_number->set_type(Person::HOME);

        person->set_data(-240000000);
        person->set_data32(6);
        person->set_data64(-1820000000);
        person->set_datadouble(123.456);
        person->set_datadouble_scientific(1.23456789e+08);
        person->set_datafloat_scientific(1.23456789e+08);
        person->set_datafloat(8.6123);
        person->set_datau32(60);
        person->set_datau64(960);
        person->set_databool(0);
        person->set_databyte("welcome to china");
        person->set_datafix32(1);
        person->set_datafix64(666);
        person->set_datasfix32(120);
        person->set_datasfix64(-802);

        std::string info1;

        google::protobuf::TextFormat::Printer printer;
        std::string text;
        printer.PrintToString(*person, &text);

        printf("text:%s\n", text.data());

        turbo::Status ret;
        printf("----------test pb to json------------\n\n");
        {
            merak::Pb2JsonOptions option;
            option.bytes_to_base64 = false;
            ret = proto_message_to_json(address_book, &info1, option);
            ASSERT_TRUE(ret.ok());
        }


        ASSERT_STREQ("{\"person\":[{\"name\":\"baidu \\u0001test\\b\\u001Aend\",\"id\":100,\"email\":"
                     "\"welcome@baidu.com\","
                     "\"phone\":[{\"number\":\"number123\",\"type\":\"HOME\"}],\"data\":-240000000,"
                     "\"data32\":6,\"data64\":-1820000000,\"datadouble\":123.456,"
                     "\"datafloat\":8.612299919128418,\"datau32\":60,\"datau64\":960,"
                     "\"databool\":false,\"databyte\":\"welcome to china\",\"datafix32\":1,"
                     "\"datafix64\":666,\"datasfix32\":120,\"datasfix64\":-802,"
                     "\"datafloat_scientific\":123456792.0,\"datadouble_scientific\":123456789.0}]}",
                     info1.data());
        std::string compatible_str = "{\"person\":[{\"name\":\"baidu \\u0001test\\b\\u001Aend\",\"id\":100,\"email\":"
        "\"welcome@baidu.com\","
        "\"phone\":[{\"number\":\"number123\",\"type\":\"HOME\"}],\"data\":-240000000,"
        "\"data32\":6,\"data64\":-1820000000,\"datadouble\":123.456,"
        "\"datafloat\":8.612299919128418,\"datau32\":60,\"datau64\":960,"
        "\"databool\":false,\"databyte\":\"welcome to china\",\"datafix32\":1,"
        "\"datafix64\":666,\"datasfix32\":120,\"datasfix64\":-802,"
        "\"datafloatScientific\":123456792.0,\"datadoubleScientific\":123456789.0}]}";

        {
            merak::Json2PbOptions json_option;
            json_option.base64_to_bytes = false;
            json_option.compatible_json_field = true;
            AddressBook book;
            ret = json_to_proto_message(compatible_str, &book, json_option, nullptr);
            //ASSERT_EQ(book.person()[0].datafloat_scientific(), 123456792.0);
            ASSERT_TRUE(ret.ok());

            merak::Pb2JsonOptions option;
            option.bytes_to_base64 = false;
            std::string info_compatible;
            ret = proto_message_to_json(book, &info_compatible, option);
            ASSERT_TRUE(ret.ok());
            ASSERT_EQ(info_compatible, info1);
        }


        info1.clear();
        {
            merak::Pb2JsonOptions option;
            option.bytes_to_base64 = true;
            ret = proto_message_to_json(address_book, &info1, option);
            ASSERT_TRUE(ret.ok());
        }

        ASSERT_STREQ("{\"person\":[{\"name\":\"baidu \\u0001test\\b\\u001Aend\",\"id\":100,\"email\":"
                     "\"welcome@baidu.com\","
                     "\"phone\":[{\"number\":\"number123\",\"type\":\"HOME\"}],\"data\":-240000000,"
                     "\"data32\":6,\"data64\":-1820000000,\"datadouble\":123.456,"
                     "\"datafloat\":8.612299919128418,\"datau32\":60,\"datau64\":960,"
                     "\"databool\":false,\"databyte\":\"d2VsY29tZSB0byBjaGluYQ==\",\"datafix32\":1,"
                     "\"datafix64\":666,\"datasfix32\":120,\"datasfix64\":-802,"
                     "\"datafloat_scientific\":123456792.0,\"datadouble_scientific\":123456789.0}]}",
                     info1.data());


        info1.clear();
        {
            merak::Pb2JsonOptions option;
            option.enum_option = merak::OUTPUT_ENUM_BY_NUMBER;
            option.bytes_to_base64 = false;
            ret = proto_message_to_json(address_book, &info1, option);
            ASSERT_TRUE(ret.ok());
        }

        ASSERT_STREQ("{\"person\":[{\"name\":\"baidu \\u0001test\\b\\u001Aend\",\"id\":100,\"email\":"
                     "\"welcome@baidu.com\","
                     "\"phone\":[{\"number\":\"number123\",\"type\":1}],\"data\":-240000000,"
                     "\"data32\":6,\"data64\":-1820000000,\"datadouble\":123.456,"
                     "\"datafloat\":8.612299919128418,\"datau32\":60,\"datau64\":960,"
                     "\"databool\":false,\"databyte\":\"welcome to china\",\"datafix32\":1,"
                     "\"datafix64\":666,\"datasfix32\":120,\"datasfix64\":-802,"
                     "\"datafloat_scientific\":123456792.0,\"datadouble_scientific\":123456789.0}]}",
                     info1.data());

    }

    TEST_F(ProtobufJsonTest, pb_to_json_unicode_case) {
        AddressBook address_book;

        Person *person = address_book.add_person();

        person->set_id(100);

        char name[255 * 1024];
        for (int j = 0; j < 1024; j++) {
            for (int i = 0; i < 255; i++) {
                name[j * 255 + i] = i + 1;
            }
        }
        name[255 * 1024 - 1] = '\0';
        person->set_name(name);
        person->set_data(-240000000);
        person->set_data32(6);
        person->set_data64(-1820000000);
        person->set_datadouble(123.456);
        person->set_datadouble_scientific(1.23456789e+08);
        person->set_datafloat_scientific(1.23456789e+08);
        person->set_datafloat(8.6123);
        person->set_datau32(60);
        person->set_datau64(960);
        person->set_databool(0);
        person->set_databyte("welcome to china");
        person->set_datafix32(1);
        person->set_datafix64(666);
        person->set_datasfix32(120);
        person->set_datasfix64(-802);

        std::string info1;
        std::string error;

        google::protobuf::TextFormat::Printer printer;
        std::string text;
        printer.PrintToString(*person, &text);

        printf("----------test pb to json------------\n\n");
        auto ret = merak::proto_message_to_json(address_book, &info1);
        ASSERT_TRUE(ret.ok());
        merak::CordOutputStream stream;
        auto res = merak::proto_message_to_json(address_book, &stream);
        ASSERT_TRUE(res.ok());
        ASSERT_TRUE(!info1.compare(stream.consume().flatten()));
    }

    TEST_F(ProtobufJsonTest, pb_to_json_edge_case) {
        AddressBook address_book;

        std::string info1;
        std::string error;
        auto ret = merak::proto_message_to_json(address_book, &info1);
        ASSERT_TRUE(ret.ok());

        info1.clear();
        Person *person = address_book.add_person();

        person->set_id(100);
        person->set_name("baidu");

        Person::PhoneNumber *phone_number = person->add_phone();
        phone_number->set_number("1234556");

        person->set_datadouble(-345.67);
        person->set_datafloat(8.6123);

        ret = merak::proto_message_to_json(address_book, &info1);
        //merak::json::Document doc;
        //ret = merak::proto_message_to_json(address_book, doc);

        std::cout<<info1<<std::endl;
        //std::cout<<doc.to_string()<<std::endl;
        //ASSERT_TRUE(false);
        ASSERT_TRUE(ret.ok());
        ASSERT_TRUE(error.empty());

        std::string info3;
        AddressBook data1;
        auto ret1 = merak::json_to_proto_message(info1, &data1);
        ASSERT_TRUE(ret1.ok());

        auto ret2 = merak::proto_message_to_json(data1, &info3);
        ASSERT_TRUE(ret2.ok());
        ASSERT_TRUE(ret2.message().empty());
    }

    TEST_F(ProtobufJsonTest, pb_to_json_expected_failed_case) {
        AddressBook address_book;
        std::string info1;
        std::string error;

        Person *person = address_book.add_person();

        person->set_name("baidu");
        person->set_email("welcome@baidu.com");

        auto ret = merak::proto_message_to_json(address_book, &info1);
        ASSERT_FALSE(ret.ok());
        ASSERT_STREQ("Missing required field: addressbook.Person.id", ret.message().c_str());

        address_book.clear_person();
        person = address_book.add_person();

        person->set_id(2);
        person->set_email("welcome@baidu.com");

        ret = merak::proto_message_to_json(address_book, &info1);
        ASSERT_FALSE(ret.ok());
        ASSERT_STREQ("Missing required field: addressbook.Person.name", ret.message().c_str());

        address_book.clear_person();
        person = address_book.add_person();

        person->set_id(2);
        person->set_name("name");
        person->set_email("welcome@baidu.com");

        ret = merak::proto_message_to_json(address_book, &info1);
        ASSERT_FALSE(ret.ok());
        ASSERT_STREQ("Missing required field: addressbook.Person.datadouble", ret.message().c_str());
    }

    TEST_F(ProtobufJsonTest, pb_to_json_perf_case) {
        AddressBook address_book;

        Person *person = address_book.add_person();

        person->set_id(100);
        person->set_name("baidu");
        person->set_email("welcome@baidu.com");

        Person::PhoneNumber *phone_number = person->add_phone();
        phone_number->set_number("number123");
        phone_number->set_type(Person::HOME);

        person->set_data(-240000000);

        person->set_data32(6);

        person->set_data64(-1820000000);

        person->set_datadouble(123.456);

        person->set_datadouble_scientific(1.23456789e+08);

        person->set_datafloat_scientific(1.23456789e+08);

        person->set_datafloat(8.6123);

        person->set_datau32(60);

        person->set_datau64(960);

        person->set_databool(0);

        person->set_databyte("welcome to china");

        person->set_datafix32(1);

        person->set_datafix64(666);

        person->set_datasfix32(120);

        person->set_datasfix64(-802);

        std::string info1;

        google::protobuf::TextFormat::Printer printer;
        std::string text;
        printer.PrintToString(*person, &text);

        printf("text:%s\n", text.data());

        printf("----------test pb to json performance------------\n\n");
        ProfilerStart("pb_to_json_perf.prof");
        turbo::TimeCost timer;
        turbo::Status res;
        float avg_time1 = 0;
        float avg_time2 = 0;
        const int times = 100000;
        ASSERT_TRUE(merak::proto_message_to_json(address_book, &info1).ok());
        for (int i = 0; i < times; i++) {
            std::string info3;
            AddressBook data1;
            timer.reset();
            res = merak::json_to_proto_message(info1, &data1);
            timer.stop();
            avg_time1 += timer.u_elapsed();
            ASSERT_TRUE(res.ok());

            timer.reset();
            res = merak::proto_message_to_json(data1, &info3);
            timer.stop();
            avg_time2 += timer.u_elapsed();
            ASSERT_TRUE(res.ok());
        }
        avg_time1 /= times;
        avg_time2 /= times;
        ProfilerStop();
        printf("avg time to convert json to pb is %fus\n", avg_time1);
        printf("avg time to convert pb to json is %fus\n", avg_time2);
    }

    TEST_F(ProtobufJsonTest, pb_to_json_encode_decode_perf_case) {
        JsonContextBodyEncDec json_data;
        json_data.set_type(80000);
        json_data.add_data_z058_array(200);
        json_data.add_data_z058_array(300);
        json_data.add_info("this is json data's info");
        json_data.add_info("this is a test");
        json_data.set_judge(true);
        json_data.set_spur(3.45);

        ContentEncDec *content = json_data.add__z064_content_test_z037__z064_();
        content->set_uid_z042_("content info");
        content->set_distance_info_(1234.56);

        ExtEncDec *ext = content->mutable__ext_z037_t_();
        ext->set_aa_ge_z040_(160000);
        ext->set_databyte_z040_std_z058__z058_string_z041_("databyte");
        ext->set_enum_z045__z045_type(ExtEncDec_PhoneTypeEncDec_WORK);

        std::string info1;

        google::protobuf::TextFormat::Printer printer;
        std::string text;
        printer.PrintToString(json_data, &text);

        printf("text:%s\n", text.data());

        ASSERT_TRUE(merak::proto_message_to_json(json_data, &info1).ok());

        printf("----------test pb to json encode decode performance------------\n\n");
        ProfilerStart("pb_to_json_encode_decode_perf.prof");
        turbo::TimeCost timer;
        turbo::Status res;
        float avg_time1 = 0;
        float avg_time2 = 0;
        const int times = 100000;
        for (int i = 0; i < times; i++) {
            std::string info3;
            JsonContextBody json_body;
            timer.reset();
            res = merak::json_to_proto_message(info1, &json_body);
            timer.stop();
            avg_time1 += timer.u_elapsed();
            ASSERT_TRUE(res.ok());

            timer.reset();
            res = merak::proto_message_to_json(json_body, &info3);
            timer.stop();
            avg_time2 += timer.u_elapsed();
            ASSERT_TRUE(res.ok());
        }
        avg_time1 /= times;
        avg_time2 /= times;
        ProfilerStop();
        printf("avg time to convert json to pb is %fus\n", avg_time1);
        printf("avg time to convert pb to json is %fus\n", avg_time2);
    }

    TEST_F(ProtobufJsonTest, pb_to_json_complex_perf_case) {

        std::ifstream in(merak::test_jsonout, std::ios::in);
        std::ostringstream tmp;
        tmp << in.rdbuf();
        std::string info3 = tmp.str();
        in.close();

        printf("----------test pb to json performance------------\n\n");

        turbo::TimeCost timer;
        turbo::Status res;
        float avg_time2 = 0;
        const int times = 10000;
        gss::message::gss_us_res_t data;
        merak::Json2PbOptions option;
        option.base64_to_bytes = false;
        res = merak::json_to_proto_message(info3, &data, option);
        ASSERT_TRUE(res.ok()) << res;
        ProfilerStart("pb_to_json_complex_perf.prof");
        for (int i = 0; i < times; i++) {
            std::string error1;
            timer.reset();
            turbo::Cord buf;
            merak::CordOutputStream stream;
            res = merak::proto_message_to_json(data, &stream);
            timer.stop();
            avg_time2 += timer.u_elapsed();
            ASSERT_TRUE(res.ok());
        }
        avg_time2 /= times;
        ProfilerStop();
        printf("avg time to convert pb to json is %fus\n", avg_time2);
    }

    TEST_F(ProtobufJsonTest, pb_to_json_to_string_complex_perf_case) {
        std::ifstream in(merak::test_jsonout, std::ios::in);
        std::ostringstream tmp;
        tmp << in.rdbuf();
        std::string info3 = tmp.str();
        in.close();

        printf("----------test pb to json performance------------\n\n");

        std::string error;

        turbo::TimeCost timer;
        turbo::Status res;
        float avg_time2 = 0;
        const int times = 10000;
        gss::message::gss_us_res_t data;
        merak::Json2PbOptions option;
        option.base64_to_bytes = false;
        res = merak::json_to_proto_message(info3, &data, option);
        ASSERT_TRUE(res.ok());
        ProfilerStart("pb_to_json_to_string_complex_perf.prof");
        for (int i = 0; i < times; i++) {
            std::string info4;
            std::string error1;
            timer.reset();
            std::string inf4;
            res = merak::proto_message_to_json(data, &info4);
            timer.stop();
            avg_time2 += timer.u_elapsed();
            ASSERT_TRUE(res.ok());
        }
        avg_time2 /= times;
        ProfilerStop();
        printf("avg time to convert pb to json is %fus\n", avg_time2);
    }

    TEST_F(ProtobufJsonTest, encode_decode_case) {

        std::string json_key = "abcdek123lske_slkejfl_l1kdle";
        std::string field_name;
        ASSERT_FALSE(merak::encode_name(json_key, field_name));
        ASSERT_TRUE(field_name.empty());
        std::string json_key_decode;
        ASSERT_FALSE(merak::decode_name(field_name, json_key_decode));
        ASSERT_TRUE(json_key_decode.empty());

        json_key = "_Afledk2e*_+%leGi___hE_Z278_t#";
        field_name.clear();
        merak::encode_name(json_key, field_name);
        const char *encode_json_key = "_Afledk2e_Z042___Z043__Z037_leGi___hE_Z278_t_Z035_";
        ASSERT_TRUE(strcmp(field_name.data(), encode_json_key) == 0);
        json_key_decode.clear();
        merak::decode_name(field_name, json_key_decode);
        ASSERT_TRUE(strcmp(json_key_decode.data(), json_key.data()) == 0);

        json_key = "_ext%T_";
        field_name.clear();
        merak::encode_name(json_key, field_name);
        encode_json_key = "_ext_Z037_T_";
        ASSERT_TRUE(strcmp(field_name.data(), encode_json_key) == 0);
        json_key_decode.clear();
        merak::decode_name(field_name, json_key_decode);
        ASSERT_TRUE(strcmp(json_key_decode.data(), json_key.data()) == 0);

        std::string empty_key;
        std::string empty_result;
        ASSERT_FALSE(merak::encode_name(empty_key, empty_result));
        ASSERT_TRUE(empty_result.empty() == true);
        ASSERT_FALSE(merak::decode_name(empty_result, empty_key));
        ASSERT_TRUE(empty_key.empty() == true);
    }

    TEST_F(ProtobufJsonTest, json_to_zero_copy_stream_normal_case) {
        Person person;
        person.set_name("hello");
        person.set_id(9);
        person.set_datadouble(2.2);
        person.set_datafloat(1);
        merak::CordOutputStream wrapper;
        ASSERT_TRUE(merak::proto_message_to_json(person, &wrapper).ok());
        auto out = wrapper.consume();
        ASSERT_EQ("{\"name\":\"hello\",\"id\":9,\"datadouble\":2.2,\"datafloat\":1.0}", out);
    }

    TEST_F(ProtobufJsonTest, zero_copy_stream_to_json_normal_case) {
        turbo::Cord iobuf;
        iobuf = "{\"name\":\"hello\",\"id\":9,\"datadouble\":2.2,\"datafloat\":1.0}";
        merak::CordInputStream wrapper(&iobuf);
        Person person;
        ASSERT_TRUE(merak::json_to_proto_message(&wrapper, &person).ok());
        ASSERT_STREQ("hello", person.name().c_str());
        ASSERT_EQ(9, person.id());
        ASSERT_EQ(2.2, person.datadouble());
        ASSERT_EQ(1, person.datafloat());
    }

    TEST_F(ProtobufJsonTest, extension_case) {
        std::string json = "{\"name\":\"hello\",\"id\":9,\"datadouble\":2.2,\"datafloat\":1.0,\"hobby\":\"coding\"}";
        Person person;
        ASSERT_TRUE(merak::json_to_proto_message(json, &person).ok());
        ASSERT_STREQ("coding", person.GetExtension(addressbook::hobby).data());
        std::string output;
        ASSERT_TRUE(merak::proto_message_to_json(person, &output).ok());
        ASSERT_EQ("{\"hobby\":\"coding\",\"name\":\"hello\",\"id\":9,\"datadouble\":2.2,\"datafloat\":1.0}", output);
    }

    TEST_F(ProtobufJsonTest, string_to_int64) {
        auto json = R"({"name":"hello", "id":9, "data": "123456", "datadouble":2.2, "datafloat":1.0})";
        Person person;
        ASSERT_TRUE(merak::json_to_proto_message(json, &person).ok());
        ASSERT_EQ(person.data(), 123456);
        json = R"({"name":"hello", "id":9, "data": 1234567, "datadouble":2.2, "datafloat":1.0})";
        ASSERT_TRUE(merak::json_to_proto_message(json, &person).ok());
        ASSERT_EQ(person.data(), 1234567);
    }

    TEST_F(ProtobufJsonTest, parse_multiple_json) {
        const int COUNT = 4;
        std::vector<std::string> expectedNames = {"tom", "bob", "jerry", "lucy"};
        std::vector<int> expectedIds = {33, 12, 2432, 435};
        std::vector<double> expectedData = {1.0, 2.0, 3.0, 4.0};
        std::string jsonStr;
        turbo::Cord jsonBuf;
        for (int i = 0; i < COUNT; ++i) {
            const std::string d =
                    turbo::str_format(R"( { "name":"%s", "id":%d, "datadouble":%f } )",
                                      expectedNames[i].c_str(),
                                      expectedIds[i],
                                      expectedData[i]);
            jsonStr.append(d);
            jsonBuf.append(d);
        }

        Person req;
        merak::Json2PbOptions copt;
        copt.allow_remaining_bytes_after_parsing = true;

        for (int i = 0; true; ++i) {
            req.Clear();
            size_t offset;
            auto rs = merak::json_to_proto_message(jsonStr, &req, copt, &offset);
            if (rs.ok()) {
                jsonStr = jsonStr.substr(offset);
                ASSERT_EQ(expectedNames[i], req.name());
                ASSERT_EQ(expectedIds[i], req.id());
                ASSERT_EQ(expectedData[i], req.datadouble());

                std::cout << "parsed=" << req.ShortDebugString() << " after_offset=" << jsonStr << std::endl;
            } else {
                if (turbo::is_not_found(rs)) {
                    // document is empty
                    break;
                }
                std::cerr << "error=" << rs << " offset=" << offset << std::endl;
                ASSERT_FALSE(true);
            }
        }

        merak::CordInputStream stream(&jsonBuf);
        merak::ZeroCopyStreamReader reader(&stream);

        for (int i = 0; true; ++i) {
            req.Clear();
            size_t offset;
            auto res = merak::json_to_proto_message(&reader, &req, copt, &offset);
            if (res.ok()) {
                ASSERT_EQ(expectedNames[i], req.name());
                ASSERT_EQ(expectedIds[i], req.id());
                ASSERT_EQ(expectedData[i], req.datadouble());
                //std::string afterOffset;
                auto afterOffset = jsonBuf.subcord(offset,size_t(-1));
                std::cout << "parsed=" << req.ShortDebugString() << " after_offset=" << afterOffset << std::endl;
            } else {
                if (turbo::is_not_found(res)) {
                    // document is empty
                    break;
                }
                std::cerr << "error=" << res << " offset=" << offset << std::endl;
                ASSERT_FALSE(true) << i;
            }
        }
    }

    TEST_F(ProtobufJsonTest, parse_multiple_json_error) {
        std::string jsonStr = R"( { "name":"tom", "id":323, "datadouble":3.2 }  abc )";
        turbo::Cord jsonBuf;
        jsonBuf.append(jsonStr);

        Person req;
        merak::Json2PbOptions copt;
        copt.allow_remaining_bytes_after_parsing = true;
        size_t offset;

        ASSERT_TRUE(merak::json_to_proto_message(jsonStr, &req, copt,  &offset).ok());
        jsonStr = jsonStr.substr(offset);
        ASSERT_STREQ("tom", req.name().c_str());
        ASSERT_EQ(323, req.id());
        ASSERT_EQ(3.2, req.datadouble());

        req.Clear();
        auto ret = merak::json_to_proto_message(jsonStr, &req, copt,&offset);
        ASSERT_FALSE(ret.ok());
        ASSERT_STREQ("Invalid json: Invalid value. [Person]", ret.message().c_str());
        ASSERT_EQ(2ul, offset);

        merak::CordInputStream stream(&jsonBuf);
        merak::ZeroCopyStreamReader reader(&stream);
        req.Clear();
        ASSERT_TRUE(merak::json_to_proto_message(&reader, &req, copt,  &offset).ok());
        ASSERT_STREQ("tom", req.name().c_str());
        ASSERT_EQ(323, req.id());
        ASSERT_EQ(3.2, req.datadouble());

        req.Clear();
        ret = merak::json_to_proto_message(&reader, &req, copt, &offset);
        ASSERT_FALSE(ret.ok());
        ASSERT_STREQ("Invalid json: Invalid value. [Person]", ret.message().c_str());
        ASSERT_EQ(47ul, offset);
    }

} // namespace
