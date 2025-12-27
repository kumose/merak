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

// JSON to JSONx conversion example, using SAX API.
// JSONx is an IBM standard format to represent JSON as XML.
// https://www-01.ibm.com/support/knowledgecenter/SS9H2Y_7.1.0/com.ibm.dp.doc/json_jsonx.html
// This example parses JSON text from stdin with validation, 
// and convert to JSONx format to stdout.
// Need compile with -D__STDC_FORMAT_MACROS for defining PRId64 and PRIu64 macros.

#include <merak/json.h>
#include <cstdio>

using namespace merak::json;

// For simplicity, this example only read/write in UTF-8 encoding
template <typename OutputStream>
class JsonxWriter {
public:
    JsonxWriter(OutputStream& os) : os_(os), name_(), level_(0), hasName_(false) {
    }

    bool emplace_null() {
        return WriteStartElement("null", true);
    }
    
    bool emplace_bool(bool b) {
        return 
            WriteStartElement("boolean") &&
            write_string(b ? "true" : "false") &&
            WriteEndElement("boolean");
    }
    
    bool emplace_int32(int i) {
        char buffer[12];
        return WriteNumberElement(buffer, sprintf(buffer, "%d", i));
    }
    
    bool emplace_uint32(unsigned i) {
        char buffer[11];
        return WriteNumberElement(buffer, sprintf(buffer, "%u", i));
    }
    
    bool emplace_int64(int64_t i) {
        char buffer[21];
        return WriteNumberElement(buffer, sprintf(buffer, "%" PRId64, i));
    }
    
    bool emplace_uint64(uint64_t i) {
        char buffer[21];
        return WriteNumberElement(buffer, sprintf(buffer, "%" PRIu64, i));
    }
    
    bool emplace_double(double d) {
        char buffer[30];
        return WriteNumberElement(buffer, sprintf(buffer, "%.17g", d));
    }

    bool raw_number(const char* str, SizeType length, bool) {
        return
            WriteStartElement("number") &&
            WriteEscapedText(str, length) &&
            WriteEndElement("number");
    }

    bool emplace_string(const char* str, SizeType length, bool) {
        return
            WriteStartElement("string") &&
            WriteEscapedText(str, length) &&
            WriteEndElement("string");
    }

    bool start_object() {
        return WriteStartElement("object");
    }

    bool emplace_key(const char* str, SizeType length, bool) {
        // backup key to name_
        name_.clear();
        for (SizeType i = 0; i < length; i++)
            name_.put(str[i]);
        hasName_ = true;
        return true;
    }

    bool end_object(SizeType) {
        return WriteEndElement("object");
    }

    bool start_array() {
        return WriteStartElement("array");
    }

    bool end_array(SizeType) {
        return WriteEndElement("array");
    }

private:
    bool write_string(const char* s) {
        while (*s)
            os_.put(*s++);
        return true;
    }

    bool WriteEscapedAttributeValue(const char* s, size_t length) {
        for (size_t i = 0; i < length; i++) {
            switch (s[i]) {
                case '&': write_string("&amp;"); break;
                case '<': write_string("&lt;"); break;
                case '"': write_string("&quot;"); break;
                default: os_.put(s[i]); break;
            }
        }
        return true;
    }

    bool WriteEscapedText(const char* s, size_t length) {
        for (size_t i = 0; i < length; i++) {
            switch (s[i]) {
                case '&': write_string("&amp;"); break;
                case '<': write_string("&lt;"); break;
                default: os_.put(s[i]); break;
            }
        }
        return true;
    }

    bool WriteStartElement(const char* type, bool emptyElement = false) {
        if (level_ == 0)
            if (!write_string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"))
                return false;

        if (!write_string("<json:") || !write_string(type))
            return false;

        // For root element, need to add declarations
        if (level_ == 0) {
            if (!write_string(
                " xsi:schemaLocation=\"http://www.datapower.com/schemas/json jsonx.xsd\""
                " xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\""
                " xmlns:json=\"http://www.ibm.com/xmlns/prod/2009/jsonx\""))
                return false;
        }

        if (hasName_) {
            hasName_ = false;
            if (!write_string(" name=\"") ||
                !WriteEscapedAttributeValue(name_.get_string(), name_.GetSize()) ||
                !write_string("\""))
                return false;
        }

        if (emptyElement)
            return write_string("/>");
        else {
            level_++;
            return write_string(">");
        }
    }

    bool WriteEndElement(const char* type) {
        if (!write_string("</json:") ||
            !write_string(type) ||
            !write_string(">"))
            return false;

        // For the last end tag, flush the output stream.
        if (--level_ == 0)
            os_.flush();

        return true;
    }

    bool WriteNumberElement(const char* buffer, int length) {
        if (!WriteStartElement("number"))
            return false;
        for (int j = 0; j < length; j++)
            os_.put(buffer[j]);
        return WriteEndElement("number");
    }

    OutputStream& os_;
    StringBuffer name_;
    unsigned level_;
    bool hasName_;
};

int main(int, char*[]) {
    // Prepare JSON reader and input stream.
    Reader reader;
    char readBuffer[65536];
    FileReadStream is(stdin, readBuffer, sizeof(readBuffer));

    // Prepare JSON writer and output stream.
    char writeBuffer[65536];
    FileWriteStream os(stdout, writeBuffer, sizeof(writeBuffer));
    JsonxWriter<FileWriteStream> writer(os);

    // JSON reader parse from the input stream and let writer generate the output.
    if (!reader.parse(is, writer)) {
        fprintf(stderr, "\nError(%u): %s\n", static_cast<unsigned>(reader.get_error_offset()), get_parse_error_en(reader.GetParseErrorCode()));
        return 1;
    }

    return 0;
}
