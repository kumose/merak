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

#include "archiver.h"
#include <cassert>
#include <stack>
#include <merak/json.h>

using namespace merak::json;

struct JsonReaderStackItem {
    enum State {
        BeforeStart,    //!< An object/array is in the stack but it is not yet called by start_object()/start_array().
        Started,        //!< An object/array is called by start_object()/start_array().
        Closed          //!< An array is closed after read all element, but before end_array().
    };

    JsonReaderStackItem(const Value* value, State state) : value(value), state(state), index() {}

    const Value* value;
    State state;
    SizeType index;   // For array iteration
};

typedef std::stack<JsonReaderStackItem> JsonReaderStack;

#define DOCUMENT reinterpret_cast<Document*>(mDocument)
#define STACK (reinterpret_cast<JsonReaderStack*>(mStack))
#define TOP (STACK->top())
#define CURRENT (*TOP.value)

JsonReader::JsonReader(const char* json) : mDocument(), mStack(), mError(false) {
    mDocument = new Document;
    DOCUMENT->parse(json);
    if (DOCUMENT->has_parse_error())
        mError = true;
    else {
        mStack = new JsonReaderStack;
        STACK->push(JsonReaderStackItem(DOCUMENT, JsonReaderStackItem::BeforeStart));
    }
}

JsonReader::~JsonReader() {
    delete DOCUMENT;
    delete STACK;
}

// Archive concept
JsonReader& JsonReader::start_object() {
    if (!mError) {
        if (CURRENT.is_object() && TOP.state == JsonReaderStackItem::BeforeStart)
            TOP.state = JsonReaderStackItem::Started;
        else
            mError = true;
    }
    return *this;
}

JsonReader& JsonReader::end_object() {
    if (!mError) {
        if (CURRENT.is_object() && TOP.state == JsonReaderStackItem::Started)
            Next();
        else
            mError = true;
    }
    return *this;
}

JsonReader& JsonReader::Member(const char* name) {
    if (!mError) {
        if (CURRENT.is_object() && TOP.state == JsonReaderStackItem::Started) {
            Value::ConstMemberIterator memberItr = CURRENT.find_member(name);
            if (memberItr != CURRENT.member_end())
                STACK->push(JsonReaderStackItem(&memberItr->value, JsonReaderStackItem::BeforeStart));
            else
                mError = true;
        }
        else
            mError = true;
    }
    return *this;
}

bool JsonReader::has_member(const char* name) const {
    if (!mError && CURRENT.is_object() && TOP.state == JsonReaderStackItem::Started)
        return CURRENT.has_member(name);
    return false;
}

JsonReader& JsonReader::start_array(size_t* size) {
    if (!mError) {
        if (CURRENT.is_array() && TOP.state == JsonReaderStackItem::BeforeStart) {
            TOP.state = JsonReaderStackItem::Started;
            if (size)
                *size = CURRENT.size();

            if (!CURRENT.empty()) {
                const Value* value = &CURRENT[TOP.index];
                STACK->push(JsonReaderStackItem(value, JsonReaderStackItem::BeforeStart));
            }
            else
                TOP.state = JsonReaderStackItem::Closed;
        }
        else
            mError = true;
    }
    return *this;
}

JsonReader& JsonReader::end_array() {
    if (!mError) {
        if (CURRENT.is_array() && TOP.state == JsonReaderStackItem::Closed)
            Next();
        else
            mError = true;
    }
    return *this;
}

JsonReader& JsonReader::operator&(bool& b) {
    if (!mError) {
        if (CURRENT.is_bool()) {
            b = CURRENT.get_bool();
            Next();
        }
        else
            mError = true;
    }
    return *this;
}

JsonReader& JsonReader::operator&(unsigned& u) {
    if (!mError) {
        if (CURRENT.is_uint32()) {
            u = CURRENT.get_uint32();
            Next();
        }
        else
            mError = true;
    }
    return *this;
}

JsonReader& JsonReader::operator&(int& i) {
    if (!mError) {
        if (CURRENT.is_int32()) {
            i = CURRENT.get_int32();
            Next();
        }
        else
            mError = true;
    }
    return *this;
}

JsonReader& JsonReader::operator&(double& d) {
    if (!mError) {
        if (CURRENT.is_number()) {
            d = CURRENT.get_double();
            Next();
        }
        else
            mError = true;
    }
    return *this;
}

JsonReader& JsonReader::operator&(std::string& s) {
    if (!mError) {
        if (CURRENT.is_string()) {
            s = CURRENT.get_string();
            Next();
        }
        else
            mError = true;
    }
    return *this;
}

JsonReader& JsonReader::SetNull() {
    // This function is for JsonWriter only.
    mError = true;
    return *this;
}

void JsonReader::Next() {
    if (!mError) {
        assert(!STACK->empty());
        STACK->pop();

        if (!STACK->empty() && CURRENT.is_array()) {
            if (TOP.state == JsonReaderStackItem::Started) { // Otherwise means reading array item pass end
                if (TOP.index < CURRENT.size() - 1) {
                    const Value* value = &CURRENT[++TOP.index];
                    STACK->push(JsonReaderStackItem(value, JsonReaderStackItem::BeforeStart));
                }
                else
                    TOP.state = JsonReaderStackItem::Closed;
            }
            else
                mError = true;
        }
    }
}

#undef DOCUMENT
#undef STACK
#undef TOP
#undef CURRENT

////////////////////////////////////////////////////////////////////////////////
// JsonWriter

#define WRITER reinterpret_cast<PrettyWriter<StringBuffer>*>(mWriter)
#define STREAM reinterpret_cast<StringBuffer*>(mStream)

JsonWriter::JsonWriter() : mWriter(), mStream() {
    mStream = new StringBuffer;
    mWriter = new PrettyWriter<StringBuffer>(*STREAM);
}

JsonWriter::~JsonWriter() { 
    delete WRITER;
    delete STREAM;
}

const char* JsonWriter::get_string() const {
    return STREAM->get_string();
}

JsonWriter& JsonWriter::start_object() {
    WRITER->start_object();
    return *this;
}

JsonWriter& JsonWriter::end_object() {
    WRITER->end_object();
    return *this;
}

JsonWriter& JsonWriter::Member(const char* name) {
    WRITER->emplace_string(name, static_cast<SizeType>(strlen(name)));
    return *this;
}

bool JsonWriter::has_member(const char*) const {
    // This function is for JsonReader only.
    assert(false);
    return false;
}

JsonWriter& JsonWriter::start_array(size_t*) {
    WRITER->start_array();
    return *this;
}

JsonWriter& JsonWriter::end_array() {
    WRITER->end_array();
    return *this;
}

JsonWriter& JsonWriter::operator&(bool& b) {
    WRITER->emplace_bool(b);
    return *this;
}

JsonWriter& JsonWriter::operator&(unsigned& u) {
    WRITER->emplace_uint32(u);
    return *this;
}

JsonWriter& JsonWriter::operator&(int& i) {
    WRITER->emplace_int32(i);
    return *this;
}

JsonWriter& JsonWriter::operator&(double& d) {
    WRITER->emplace_double(d);
    return *this;
}

JsonWriter& JsonWriter::operator&(std::string& s) {
    WRITER->emplace_string(s.c_str(), static_cast<SizeType>(s.size()));
    return *this;
}

JsonWriter& JsonWriter::SetNull() {
    WRITER->emplace_null();
    return *this;
}

#undef STREAM
#undef WRITER
