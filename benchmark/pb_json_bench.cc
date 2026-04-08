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

#include <benchmark/benchmark.h>

#include <google/protobuf/util/json_util.h>

#include <merak/proto/json_to_pb.h>
#include <merak/proto/pb_to_json.h>
#include <benchmark/proto/addressbook.pb.h>

namespace {

using addressbook::AddressBook;
using addressbook::Person;

// Same scalar payload as tests/mock_test.cc (ProtobufJsonTest.json_to_pb_unicode_case).
AddressBook MakeSampleBook() {
    AddressBook book;
    Person* person = book.add_person();
    person->set_id(100);
    person->set_name("myname");
    person->set_data(-240000000);
    person->set_data32(6);
    person->set_data64(-1820000000);
    person->set_datadouble(123.456);
    person->set_datadouble_scientific(1.23456789e+08);
    person->set_datafloat_scientific(1.23456789e+08);
    person->set_datafloat(8.6123f);
    person->set_datau32(60);
    person->set_datau64(960);
    person->set_databool(false);
    person->set_databyte("welcome to china");
    person->set_datafix32(1);
    person->set_datafix64(666);
    person->set_datasfix32(120);
    person->set_datasfix64(-802);
    return book;
}

google::protobuf::util::JsonPrintOptions NativePrintOptions() {
    google::protobuf::util::JsonPrintOptions o;
    o.add_whitespace = false;
    o.preserve_proto_field_names = true;
    return o;
}

google::protobuf::util::JsonParseOptions NativeParseOptions() {
    google::protobuf::util::JsonParseOptions o;
    o.ignore_unknown_fields = false;
    return o;
}

void BM_Native_PbToJson(benchmark::State& state) {
    const AddressBook book = MakeSampleBook();
    google::protobuf::util::JsonPrintOptions print_opts = NativePrintOptions();
    std::string out;
    for (auto _ : state) {
        out.clear();
        const auto st =
            google::protobuf::util::MessageToJsonString(book, &out, print_opts);
        if (!st.ok()) {
            state.SkipWithError(std::string(st.message()).c_str());
            break;
        }
        benchmark::DoNotOptimize(out.data());
        benchmark::ClobberMemory();
    }
}

void BM_Merak_PbToJson(benchmark::State& state) {
    const AddressBook book = MakeSampleBook();
    std::string out;
    for (auto _ : state) {
        out.clear();
        const turbo::Status st = merak::proto_message_to_json(book, &out);
        if (!st.ok()) {
            state.SkipWithError(st.to_string().c_str());
            break;
        }
        benchmark::DoNotOptimize(out.data());
        benchmark::ClobberMemory();
    }
}

void BM_Native_JsonToPb(benchmark::State& state) {
    const AddressBook book = MakeSampleBook();
    std::string json;
    const auto enc =
        google::protobuf::util::MessageToJsonString(book, &json, NativePrintOptions());
    if (!enc.ok()) {
        state.SkipWithError(std::string(enc.message()).c_str());
        return;
    }
    google::protobuf::util::JsonParseOptions parse_opts = NativeParseOptions();
    for (auto _ : state) {
        AddressBook decoded;
        const auto st =
            google::protobuf::util::JsonStringToMessage(json, &decoded, parse_opts);
        if (!st.ok()) {
            state.SkipWithError(std::string(st.message()).c_str());
            break;
        }
        benchmark::DoNotOptimize(decoded.person_size());
        benchmark::ClobberMemory();
    }
}

void BM_Merak_JsonToPb(benchmark::State& state) {
    const AddressBook book = MakeSampleBook();
    std::string json;
    if (!merak::proto_message_to_json(book, &json).ok()) {
        state.SkipWithError("merak encode failed");
        return;
    }
    for (auto _ : state) {
        AddressBook decoded;
        const turbo::Status st = merak::json_to_proto_message(json, &decoded);
        if (!st.ok()) {
            state.SkipWithError(st.to_string().c_str());
            break;
        }
        benchmark::DoNotOptimize(decoded.person_size());
        benchmark::ClobberMemory();
    }
}

}  // namespace

BENCHMARK(BM_Native_PbToJson);
BENCHMARK(BM_Merak_PbToJson);
BENCHMARK(BM_Native_JsonToPb);
BENCHMARK(BM_Merak_JsonToPb);
