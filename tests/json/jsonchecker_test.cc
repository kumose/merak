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

#include <tests/json/unittest.h>

#include <merak/json/document.h>

using namespace merak::json;

static char* ReadFile(const char* filename, size_t& length) {
    const char *paths[] = {
        "jsonchecker",
        "tests/bin/jsonchecker",
        "../tests/bin/jsonchecker",
        "../../tests/bin/jsonchecker",
        "../../../tests/bin/jsonchecker"
    };
    char buffer[1024];
    FILE *fp = 0;
    for (size_t i = 0; i < sizeof(paths) / sizeof(paths[0]); i++) {
        sprintf(buffer, "%s/%s", paths[i], filename);
        fp = fopen(buffer, "rb");
        if (fp)
            break;
    }

    if (!fp)
        return 0;

    fseek(fp, 0, SEEK_END);
    length = static_cast<size_t>(ftell(fp));
    fseek(fp, 0, SEEK_SET);
    char* json = static_cast<char*>(malloc(length + 1));
    size_t readLength = fread(json, 1, length, fp);
    json[readLength] = '\0';
    fclose(fp);
    return json;
}

struct NoOpHandler {
    bool emplace_null() { return true; }
    bool emplace_bool(bool) { return true; }
    bool emplace_int32(int) { return true; }
    bool emplace_uint32(unsigned) { return true; }
    bool emplace_int64(int64_t) { return true; }
    bool emplace_uint64(uint64_t) { return true; }
    bool emplace_double(double) { return true; }
    bool raw_number(const char*, SizeType, bool) { return true; }
    bool emplace_string(const char*, SizeType, bool) { return true; }
    bool start_object() { return true; }
    bool emplace_key(const char*, SizeType, bool) { return true; }
    bool end_object(SizeType) { return true; }
    bool start_array() { return true; }
    bool end_array(SizeType) { return true; }
};


TEST(JsonChecker, Reader) {
    char filename[256];

    // jsonchecker/failXX.json
    for (int i = 1; i <= 33; i++) {
        if (i == 1) // fail1.json is valid in rapidjson, which has no limitation on type of root element (RFC 7159).
            continue;
        if (i == 18)    // fail18.json is valid in rapidjson, which has no limitation on depth of nesting.
            continue;

        sprintf(filename, "fail%d.json", i);
        size_t length;
        char* json = ReadFile(filename, length);
        if (!json) {
            printf("jsonchecker file %s not found", filename);
            ADD_FAILURE();
            continue;
        }

        // Test stack-based parsing.
        GenericDocument<UTF8<>, CrtAllocator> document; // Use Crt allocator to check exception-safety (no memory leak)
        document.parse(json);
        EXPECT_TRUE(document.has_parse_error()) << filename;

        // Test iterative parsing.
        document.parse<kParseIterativeFlag>(json);
        EXPECT_TRUE(document.has_parse_error()) << filename;

        // Test iterative pull-parsing.
        Reader reader;
        StringStream ss(json);
        NoOpHandler h;
        reader.IterativeParseInit();
        while (!reader.IterativeParseComplete()) {
            if (!reader.IterativeParseNext<kParseDefaultFlags>(ss, h))
                break;
        }
        EXPECT_TRUE(reader.has_parse_error()) << filename;
        
        free(json);
    }

    // passX.json
    for (int i = 1; i <= 3; i++) {
        sprintf(filename, "pass%d.json", i);
        size_t length;
        char* json = ReadFile(filename, length);
        if (!json) {
            printf("jsonchecker file %s not found", filename);
            continue;
        }

        // Test stack-based parsing.
        GenericDocument<UTF8<>, CrtAllocator> document; // Use Crt allocator to check exception-safety (no memory leak)
        document.parse(json);
        EXPECT_FALSE(document.has_parse_error()) << filename;

        // Test iterative parsing.
        document.parse<kParseIterativeFlag>(json);
        EXPECT_FALSE(document.has_parse_error()) << filename;
        
        // Test iterative pull-parsing.
        Reader reader;
        StringStream ss(json);
        NoOpHandler h;
        reader.IterativeParseInit();
        while (!reader.IterativeParseComplete()) {
            if (!reader.IterativeParseNext<kParseDefaultFlags>(ss, h))
                break;
        }
        EXPECT_FALSE(reader.has_parse_error()) << filename;

        free(json);
    }
}
