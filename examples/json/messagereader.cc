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
