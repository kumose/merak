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
// JSON condenser example

// This example parses JSON from stdin with validation, 
// and re-output the JSON content to stdout with all string capitalized, and without whitespace.

#include <merak/json.h>
#include <vector>
#include <cctype>

using namespace merak::json;

template<typename OutputHandler>
struct CapitalizeFilter {
    CapitalizeFilter(OutputHandler& out) : out_(out), buffer_() {}

    bool emplace_null() { return out_.emplace_null(); }
    bool emplace_bool(bool b) { return out_.emplace_bool(b); }
    bool emplace_int32(int i) { return out_.emplace_int32(i); }
    bool emplace_uint32(unsigned u) { return out_.emplace_uint32(u); }
    bool emplace_int64(int64_t i) { return out_.emplace_int64(i); }
    bool emplace_uint64(uint64_t u) { return out_.emplace_uint64(u); }
    bool emplace_double(double d) { return out_.emplace_double(d); }
    bool raw_number(const char* str, SizeType length, bool copy) { return out_.raw_number(str, length, copy); }
    bool emplace_string(const char* str, SizeType length, bool) {
        buffer_.clear();
        for (SizeType i = 0; i < length; i++)
            buffer_.push_back(static_cast<char>(std::toupper(str[i])));
        return out_.emplace_string(&buffer_.front(), length, true); // true = output handler need to copy the string
    }
    bool start_object() { return out_.start_object(); }
    bool emplace_key(const char* str, SizeType length, bool copy) { return emplace_string(str, length, copy); }
    bool end_object(SizeType memberCount) { return out_.end_object(memberCount); }
    bool start_array() { return out_.start_array(); }
    bool end_array(SizeType elementCount) { return out_.end_array(elementCount); }

    OutputHandler& out_;
    std::vector<char> buffer_;

private:
    CapitalizeFilter(const CapitalizeFilter&);
    CapitalizeFilter& operator=(const CapitalizeFilter&);
};

int main(int, char*[]) {
    // Prepare JSON reader and input stream.
    Reader reader;
    char readBuffer[65536];
    FileReadStream is(stdin, readBuffer, sizeof(readBuffer));

    // Prepare JSON writer and output stream.
    char writeBuffer[65536];
    FileWriteStream os(stdout, writeBuffer, sizeof(writeBuffer));
    Writer<FileWriteStream> writer(os);

    // JSON reader parse from the input stream and let writer generate the output.
    CapitalizeFilter<Writer<FileWriteStream> > filter(writer);
    if (!reader.parse(is, filter)) {
        fprintf(stderr, "\nError(%u): %s\n", static_cast<unsigned>(reader.get_error_offset()), get_parse_error_en(reader.GetParseErrorCode()));
        return 1;
    }

    return 0;
}
