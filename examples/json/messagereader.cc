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
// Copyright (C) 2015 THL A29 Limited, a Tencent company, and Milo Yip.
// Reading a message JSON with Reader (SAX-style API).
// The JSON should be an object with key-string pairs.

#include <merak/json.h>
#include <iostream>
#include <string>
#include <map>

using namespace std;
using namespace merak::json;

typedef map<string, string> MessageMap;

#if defined(__GNUC__)
MERAK_JSON_DIAG_PUSH
MERAK_JSON_DIAG_OFF(effc++)
#endif

#ifdef __clang__
MERAK_JSON_DIAG_PUSH
MERAK_JSON_DIAG_OFF(switch-enum)
#endif

struct MessageHandler
    : public BaseReaderHandler<UTF8<>, MessageHandler> {
    MessageHandler() : messages_(), state_(kExpectObjectStart), name_() {}

    bool start_object() {
        switch (state_) {
        case kExpectObjectStart:
            state_ = kExpectNameOrObjectEnd;
            return true;
        default:
            return false;
        }
    }

    bool emplace_string(const char* str, SizeType length, bool) {
        switch (state_) {
        case kExpectNameOrObjectEnd:
            name_ = string(str, length);
            state_ = kExpectValue;
            return true;
        case kExpectValue:
            messages_.insert(MessageMap::value_type(name_, string(str, length)));
            state_ = kExpectNameOrObjectEnd;
            return true;
        default:
            return false;
        }
    }

    bool end_object(SizeType) { return state_ == kExpectNameOrObjectEnd; }

    bool Default() { return false; } // All other events are invalid.

    MessageMap messages_;
    enum State {
        kExpectObjectStart,
        kExpectNameOrObjectEnd,
        kExpectValue
    }state_;
    std::string name_;
};

#if defined(__GNUC__)
MERAK_JSON_DIAG_POP
#endif

#ifdef __clang__
MERAK_JSON_DIAG_POP
#endif

static void ParseMessages(const char* json, MessageMap& messages) {
    Reader reader;
    MessageHandler handler;
    StringStream ss(json);
    if (reader.parse(ss, handler))
        messages.swap(handler.messages_);   // Only change it if success.
    else {
        ParseErrorCode e = reader.GetParseErrorCode();
        size_t o = reader.get_error_offset();
        cout << "Error: " << get_parse_error_en(e) << endl;;
        cout << " at offset " << o << " near '" << string(json).substr(o, 10) << "...'" << endl;
    }
}

int main() {
    MessageMap messages;

    const char* json1 = "{ \"greeting\" : \"Hello!\", \"farewell\" : \"bye-bye!\" }";
    cout << json1 << endl;
    ParseMessages(json1, messages);

    for (MessageMap::const_iterator itr = messages.begin(); itr != messages.end(); ++itr)
        cout << itr->first << ": " << itr->second << endl;

    cout << endl << "parse a JSON with invalid schema." << endl;
    const char* json2 = "{ \"greeting\" : \"Hello!\", \"farewell\" : \"bye-bye!\", \"foo\" : {} }";
    cout << json2 << endl;
    ParseMessages(json2, messages);

    return 0;
}
