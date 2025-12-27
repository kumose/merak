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

#include <merak/json.h>
#include <iostream>

MERAK_JSON_DIAG_PUSH
#ifdef __GNUC__
MERAK_JSON_DIAG_OFF(effc++)
#endif

// This example demonstrates JSON token-by-token parsing with an API that is
// more direct; you don't need to design your logic around a handler object and
// callbacks. Instead, you retrieve values from the JSON stream by calling
// get_int32(), get_double(), get_string() and get_bool(), traverse into structures
// by calling EnterObject() and EnterArray(), and skip over unwanted data by
// calling SkipValue(). When you know your JSON's structure, this can be quite
// convenient.
//
// If you aren't sure of what's next in the JSON data, you can use PeekType() and
// PeekValue() to look ahead to the next object before reading it.
//
// If you call the wrong retrieval method--e.g. get_int32 when the next JSON token is
// not an int, EnterObject or EnterArray when there isn't actually an object or array
// to read--the stream parsing will end immediately and no more data will be delivered.
//
// After calling EnterObject, you retrieve keys via NextObjectKey() and values via
// the normal getters. When NextObjectKey() returns null, you have exited the
// object, or you can call SkipObject() to skip to the end of the object
// immediately. If you fetch the entire object (i.e. NextObjectKey() returned  null),
// you should not call SkipObject().
//
// After calling EnterArray(), you must alternate between calling NextArrayValue()
// to see if the array has more data, and then retrieving values via the normal
// getters. You can call SkipArray() to skip to the end of the array immediately.
// If you fetch the entire array (i.e. NextArrayValue() returned null),
// you should not call SkipArray().
//
// This parser uses in-situ strings, so the JSON buffer will be altered during the
// parse.

using namespace merak::json;


class LookaheadParserHandler {
public:
    bool emplace_null() { st_ = kHasNull; v_.SetNull(); return true; }
    bool emplace_bool(bool b) { st_ = kHasBool; v_.SetBool(b); return true; }
    bool emplace_int32(int i) { st_ = kHasNumber; v_.set_int32(i); return true; }
    bool emplace_uint32(unsigned u) { st_ = kHasNumber; v_.set_uint32(u); return true; }
    bool emplace_int64(int64_t i) { st_ = kHasNumber; v_.set_int64(i); return true; }
    bool emplace_uint64(uint64_t u) { st_ = kHasNumber; v_.set_uint64(u); return true; }
    bool emplace_double(double d) { st_ = kHasNumber; v_.set_double(d); return true; }
    bool raw_number(const char*, SizeType, bool) { return false; }
    bool emplace_string(const char* str, SizeType length, bool) { st_ = kHasString; v_.set_string(str, length); return true; }
    bool start_object() { st_ = kEnteringObject; return true; }
    bool emplace_key(const char* str, SizeType length, bool) { st_ = kHasKey; v_.set_string(str, length); return true; }
    bool end_object(SizeType) { st_ = kExitingObject; return true; }
    bool start_array() { st_ = kEnteringArray; return true; }
    bool end_array(SizeType) { st_ = kExitingArray; return true; }

protected:
    LookaheadParserHandler(char* str);
    void ParseNext();

protected:
    enum LookaheadParsingState {
        kInit,
        kError,
        kHasNull,
        kHasBool,
        kHasNumber,
        kHasString,
        kHasKey,
        kEnteringObject,
        kExitingObject,
        kEnteringArray,
        kExitingArray
    };
    
    Value v_;
    LookaheadParsingState st_;
    Reader r_;
    InsituStringStream ss_;
    
    static const int parseFlags = kParseDefaultFlags;
};

LookaheadParserHandler::LookaheadParserHandler(char* str) : v_(), st_(kInit), r_(), ss_(str) {
    r_.IterativeParseInit();
    ParseNext();
}

void LookaheadParserHandler::ParseNext() {
    if (r_.has_parse_error()) {
        st_ = kError;
        return;
    }
    
    r_.IterativeParseNext<parseFlags>(ss_, *this);
}

class LookaheadParser : protected LookaheadParserHandler {
public:
    LookaheadParser(char* str) : LookaheadParserHandler(str) {}
    
    bool EnterObject();
    bool EnterArray();
    const char* NextObjectKey();
    bool NextArrayValue();
    int get_int32();
    double get_double();
    const char* get_string();
    bool get_bool();
    void GetNull();

    void SkipObject();
    void SkipArray();
    void SkipValue();
    Value* PeekValue();
    int PeekType(); // returns a merak::json::Type, or -1 for no value (at end of object/array)
    
    bool IsValid() { return st_ != kError; }
    
protected:
    void SkipOut(int depth);
};

bool LookaheadParser::EnterObject() {
    if (st_ != kEnteringObject) {
        st_  = kError;
        return false;
    }
    
    ParseNext();
    return true;
}

bool LookaheadParser::EnterArray() {
    if (st_ != kEnteringArray) {
        st_  = kError;
        return false;
    }
    
    ParseNext();
    return true;
}

const char* LookaheadParser::NextObjectKey() {
    if (st_ == kHasKey) {
        const char* result = v_.get_string();
        ParseNext();
        return result;
    }
    
    if (st_ != kExitingObject) {
        st_ = kError;
        return 0;
    }
    
    ParseNext();
    return 0;
}

bool LookaheadParser::NextArrayValue() {
    if (st_ == kExitingArray) {
        ParseNext();
        return false;
    }
    
    if (st_ == kError || st_ == kExitingObject || st_ == kHasKey) {
        st_ = kError;
        return false;
    }

    return true;
}

int LookaheadParser::get_int32() {
    if (st_ != kHasNumber || !v_.is_int32()) {
        st_ = kError;
        return 0;
    }

    int result = v_.get_int32();
    ParseNext();
    return result;
}

double LookaheadParser::get_double() {
    if (st_ != kHasNumber) {
        st_  = kError;
        return 0.;
    }
    
    double result = v_.get_double();
    ParseNext();
    return result;
}

bool LookaheadParser::get_bool() {
    if (st_ != kHasBool) {
        st_  = kError;
        return false;
    }
    
    bool result = v_.get_bool();
    ParseNext();
    return result;
}

void LookaheadParser::GetNull() {
    if (st_ != kHasNull) {
        st_  = kError;
        return;
    }

    ParseNext();
}

const char* LookaheadParser::get_string() {
    if (st_ != kHasString) {
        st_  = kError;
        return 0;
    }
    
    const char* result = v_.get_string();
    ParseNext();
    return result;
}

void LookaheadParser::SkipOut(int depth) {
    do {
        if (st_ == kEnteringArray || st_ == kEnteringObject) {
            ++depth;
        }
        else if (st_ == kExitingArray || st_ == kExitingObject) {
            --depth;
        }
        else if (st_ == kError) {
            return;
        }

        ParseNext();
    }
    while (depth > 0);
}

void LookaheadParser::SkipValue() {
    SkipOut(0);
}

void LookaheadParser::SkipArray() {
    SkipOut(1);
}

void LookaheadParser::SkipObject() {
    SkipOut(1);
}

Value* LookaheadParser::PeekValue() {
    if (st_ >= kHasNull && st_ <= kHasKey) {
        return &v_;
    }
    
    return 0;
}

int LookaheadParser::PeekType() {
    if (st_ >= kHasNull && st_ <= kHasKey) {
        return v_.get_type();
    }
    
    if (st_ == kEnteringArray) {
        return kArrayType;
    }
    
    if (st_ == kEnteringObject) {
        return kObjectType;
    }

    return -1;
}

//-------------------------------------------------------------------------

int main() {
    using namespace std;

    char json[] = " { \"hello\" : \"world\", \"t\" : true , \"f\" : false, \"n\": null,"
        "\"i\":123, \"pi\": 3.1416, \"a\":[-1, 2, 3, 4, \"array\", []], \"skipArrays\":[1, 2, [[[3]]]], "
        "\"skipObject\":{ \"i\":0, \"t\":true, \"n\":null, \"d\":123.45 }, "
        "\"skipNested\":[[[[{\"\":0}, {\"\":[-9.87]}]]], [], []], "
        "\"skipString\":\"zzz\", \"reachedEnd\":null, \"t\":true }";

    LookaheadParser r(json);
    
    MERAK_JSON_ASSERT(r.PeekType() == kObjectType);

    r.EnterObject();
    while (const char* key = r.NextObjectKey()) {
        if (0 == strcmp(key, "hello")) {
            MERAK_JSON_ASSERT(r.PeekType() == kStringType);
            cout << key << ":" << r.get_string() << endl;
        }
        else if (0 == strcmp(key, "t") || 0 == strcmp(key, "f")) {
            MERAK_JSON_ASSERT(r.PeekType() == kTrueType || r.PeekType() == kFalseType);
            cout << key << ":" << r.get_bool() << endl;
            continue;
        }
        else if (0 == strcmp(key, "n")) {
            MERAK_JSON_ASSERT(r.PeekType() == kNullType);
            r.GetNull();
            cout << key << endl;
            continue;
        }
        else if (0 == strcmp(key, "pi")) {
            MERAK_JSON_ASSERT(r.PeekType() == kNumberType);
            cout << key << ":" << r.get_double() << endl;
            continue;
        }
        else if (0 == strcmp(key, "a")) {
            MERAK_JSON_ASSERT(r.PeekType() == kArrayType);
            
            r.EnterArray();
            
            cout << key << ":[ ";
            while (r.NextArrayValue()) {
                if (r.PeekType() == kNumberType) {
                    cout << r.get_double() << " ";
                }
                else if (r.PeekType() == kStringType) {
                    cout << r.get_string() << " ";
                }
                else {
                    r.SkipArray();
                    break;
                }
            }
            
            cout << "]" << endl;
        }
        else {
            cout << key << ":skipped" << endl;
            r.SkipValue();
        }
    }
    
    return 0;
}

MERAK_JSON_DIAG_POP
