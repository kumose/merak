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
#include <merak/proto/pb_to_flat.h>
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

namespace merak {


    class ProtobufJsonTest : public testing::Test {
    protected:
        void SetUp() {}

        void TearDown() {}
    };

    using addressbook::AddressBook;
    using addressbook::Person;

    TEST_F(ProtobufJsonTest, json_to_pb_unicode_case) {
        AddressBook address_book;

        Person *person = address_book.add_person();

        person->set_id(100);

        person->set_name("myname");
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
        merak::proto_message_to_flat_json(address_book, &info1);
        std::cout << info1 << std::endl;

        merak::FlatValueMap  map;
        merak::proto_message_to_flat(address_book, map);
        std::cout << map << std::endl;


        turbo::flat_hash_map<std::string, std::string>  fmap;
        merak::proto_message_to_flat(address_book, fmap);
        std::cout << fmap << std::endl;

        //ASSERT_TRUE(ret.ok());
        /*
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
        */
    }

}