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
// JSON filterkey example with SAX-style API.

// This example parses JSON text from stdin with validation.
// During parsing, specified key will be filtered using a SAX handler.
// It re-output the JSON content to stdout without whitespace.

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

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "filterkey key < input.json > output.json\n");
        return 1;
    }

    // Prepare JSON reader and input stream.
    Reader reader;
    char readBuffer[65536];
    FileReadStream is(stdin, readBuffer, sizeof(readBuffer));

    // Prepare JSON writer and output stream.
    char writeBuffer[65536];
    FileWriteStream os(stdout, writeBuffer, sizeof(writeBuffer));
    Writer<FileWriteStream> writer(os);

    // Prepare Filter
    FilterKeyHandler<Writer<FileWriteStream> > filter(writer, argv[1], static_cast<SizeType>(strlen(argv[1])));

    // JSON reader parse from the input stream, filter handler filters the events, and forward to writer.
    // i.e. the events flow is: reader -> filter -> writer
    if (!reader.parse(is, filter)) {
        fprintf(stderr, "\nError(%u): %s\n", static_cast<unsigned>(reader.get_error_offset()), get_parse_error_en(reader.GetParseErrorCode()));
        return 1;
    }

    return 0;
}
