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
// JSON filterkey example which populates filtered SAX events into a Document.

// This example parses JSON text from stdin with validation.
// During parsing, specified key will be filtered using a SAX handler.
// And finally the filtered events are used to populate a Document.
// As an example, the document is written to standard output.

#include <merak/json.h>
#include <stack>

using namespace merak::json;

// This handler forwards event into an output handler, with filtering the descendent events of specified key.
template <typename OutputHandler>
class FilterKeyHandler {
public:
    typedef char char_type;

    FilterKeyHandler(OutputHandler& outputHandler, const char_type* keyString, SizeType keyLength) :
        outputHandler_(outputHandler), keyString_(keyString), keyLength_(keyLength), filterValueDepth_(), filteredKeyCount_()
    {}

    bool emplace_null()             { return filterValueDepth_ > 0 ? EndValue() : outputHandler_.emplace_null()    && EndValue(); }
    bool emplace_bool(bool b)       { return filterValueDepth_ > 0 ? EndValue() : outputHandler_.emplace_bool(b)   && EndValue(); }
    bool emplace_int32(int i)         { return filterValueDepth_ > 0 ? EndValue() : outputHandler_.emplace_int32(i)    && EndValue(); }
    bool emplace_uint32(unsigned u)   { return filterValueDepth_ > 0 ? EndValue() : outputHandler_.emplace_uint32(u)   && EndValue(); }
    bool emplace_int64(int64_t i)   { return filterValueDepth_ > 0 ? EndValue() : outputHandler_.emplace_int64(i)  && EndValue(); }
    bool emplace_uint64(uint64_t u) { return filterValueDepth_ > 0 ? EndValue() : outputHandler_.emplace_uint64(u) && EndValue(); }
    bool emplace_double(double d)   { return filterValueDepth_ > 0 ? EndValue() : outputHandler_.emplace_double(d) && EndValue(); }
    bool raw_number(const char_type* str, SizeType len, bool copy) { return filterValueDepth_ > 0 ? EndValue() : outputHandler_.raw_number(str, len, copy) && EndValue(); }
    bool emplace_string   (const char_type* str, SizeType len, bool copy) { return filterValueDepth_ > 0 ? EndValue() : outputHandler_.emplace_string   (str, len, copy) && EndValue(); }
    
    bool start_object() {
        if (filterValueDepth_ > 0) {
            filterValueDepth_++;
            return true;
        }
        else {
            filteredKeyCount_.push(0);
            return outputHandler_.start_object();
        }
    }
    
    bool emplace_key(const char_type* str, SizeType len, bool copy) {
        if (filterValueDepth_ > 0) 
            return true;
        else if (len == keyLength_ && std::memcmp(str, keyString_, len) == 0) {
            filterValueDepth_ = 1;
            return true;
        }
        else {
            ++filteredKeyCount_.top();
            return outputHandler_.emplace_key(str, len, copy);
        }
    }

    bool end_object(SizeType) {
        if (filterValueDepth_ > 0) {
            filterValueDepth_--;
            return EndValue();
        }
        else {
            // Use our own filtered memberCount
            SizeType memberCount = filteredKeyCount_.top();
            filteredKeyCount_.pop();
            return outputHandler_.end_object(memberCount) && EndValue();
        }
    }

    bool start_array() {
        if (filterValueDepth_ > 0) {
            filterValueDepth_++;
            return true;
        }
        else
            return outputHandler_.start_array();
    }

    bool end_array(SizeType elementCount) {
        if (filterValueDepth_ > 0) {
            filterValueDepth_--;
            return EndValue();
        }
        else
            return outputHandler_.end_array(elementCount) && EndValue();
    }

private:
    FilterKeyHandler(const FilterKeyHandler&);
    FilterKeyHandler& operator=(const FilterKeyHandler&);

    bool EndValue() {
        if (filterValueDepth_ == 1) // Just at the end of value after filtered key
            filterValueDepth_ = 0;
        return true;
    }

    OutputHandler& outputHandler_;
    const char* keyString_;
    const SizeType keyLength_;
    unsigned filterValueDepth_;
    std::stack<SizeType> filteredKeyCount_;
};

// Implements a generator for Document::populate()
template <typename InputStream>
class FilterKeyReader {
public:
    typedef char char_type;

    FilterKeyReader(InputStream& is, const char_type* keyString, SizeType keyLength) :
        is_(is), keyString_(keyString), keyLength_(keyLength), parseResult_()
    {}

    // SAX event flow: reader -> filter -> handler
    template <typename Handler>
    bool operator()(Handler& handler) {
        FilterKeyHandler<Handler> filter(handler, keyString_, keyLength_);
        Reader reader;
        parseResult_ = reader.parse(is_, filter);
        return parseResult_;
    }

    const ParseResult& GetParseResult() const { return parseResult_; }

private:
    FilterKeyReader(const FilterKeyReader&);
    FilterKeyReader& operator=(const FilterKeyReader&);

    InputStream& is_;
    const char* keyString_;
    const SizeType keyLength_;
    ParseResult parseResult_;
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "filterkeydom key < input.json > output.json\n");
        return 1;
    }

    // Prepare input stream.
    char readBuffer[65536];
    FileReadStream is(stdin, readBuffer, sizeof(readBuffer));

    // Prepare Filter
    FilterKeyReader<FileReadStream> reader(is, argv[1], static_cast<SizeType>(strlen(argv[1])));

    // Populates the filtered events from reader
    Document document;
    document.populate(reader);
    ParseResult pr = reader.GetParseResult();
    if (!pr) {
        fprintf(stderr, "\nError(%u): %s\n", static_cast<unsigned>(pr.Offset()), get_parse_error_en(pr.Code()));
        return 1;
    }

    // Prepare JSON writer and output stream.
    char writeBuffer[65536];
    FileWriteStream os(stdout, writeBuffer, sizeof(writeBuffer));
    Writer<FileWriteStream> writer(os);

    // Write the document to standard output
    document.accept(writer);
    return 0;
}
