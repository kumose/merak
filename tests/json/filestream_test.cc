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
#include <merak/json/filereadstream.h>
#include <merak/json/filewritestream.h>
#include <merak/json/encodedstream.h>

using namespace merak::json;

class FileStreamTest : public ::testing::Test {
public:
    FileStreamTest() : filename_(), json_(), length_(), abcde_() {}
    virtual ~FileStreamTest();

    virtual void SetUp() {
        const char *paths[] = {
            "data/sample.json",
            "tests/bin/data/sample.json",
            "../tests/bin/data/sample.json",
            "../../tests/bin/data/sample.json",
            "../../../tests/bin/data/sample.json"
        };
        FILE* fp = 0;
        for (size_t i = 0; i < sizeof(paths) / sizeof(paths[0]); i++) {
            fp = fopen(paths[i], "rb");
            if (fp) {
                filename_ = paths[i];
                break;
            }
        }
        ASSERT_TRUE(fp != 0);

        fseek(fp, 0, SEEK_END);
        length_ = static_cast<size_t>(ftell(fp));
        fseek(fp, 0, SEEK_SET);
        json_ = static_cast<char*>(malloc(length_ + 1));
        size_t readLength = fread(json_, 1, length_, fp);
        json_[readLength] = '\0';
        fclose(fp);

        const char *abcde_paths[] = {
            "data/abcde.txt",
            "tests/bin/data/abcde.txt",
            "../tests/bin/data/abcde.txt",
            "../../tests/bin/data/abcde.txt",
            "../../../tests/bin/data/abcde.txt"
        };
        fp = 0;
        for (size_t i = 0; i < sizeof(abcde_paths) / sizeof(abcde_paths[0]); i++) {
            fp = fopen(abcde_paths[i], "rb");
            if (fp) {
                abcde_ = abcde_paths[i];
                break;
            }
        }
        ASSERT_TRUE(fp != 0);
        fclose(fp);
    }

    virtual void TearDown() {
        free(json_);
        json_ = 0;
    }

private:
    FileStreamTest(const FileStreamTest&);
    FileStreamTest& operator=(const FileStreamTest&);
    
protected:
    const char* filename_;
    char *json_;
    size_t length_;
    const char* abcde_;
};

FileStreamTest::~FileStreamTest() {}

TEST_F(FileStreamTest, FileReadStream) {
    FILE *fp = fopen(filename_, "rb");
    ASSERT_TRUE(fp != 0);
    char buffer[65536];
    FileReadStream s(fp, buffer, sizeof(buffer));

    for (size_t i = 0; i < length_; i++) {
        EXPECT_EQ(json_[i], s.peek());
        EXPECT_EQ(json_[i], s.peek());  // 2nd time should be the same
        EXPECT_EQ(json_[i], s.get());
    }

    EXPECT_EQ(length_, s.tellg());
    EXPECT_EQ('\0', s.peek());

    fclose(fp);
}

TEST_F(FileStreamTest, FileWriteStream) {
    char filename[L_tmpnam];
    FILE* fp = TempFile(filename);

    char buffer[65536];
    FileWriteStream os(fp, buffer, sizeof(buffer));
    for (size_t i = 0; i < length_; i++)
        os.put(json_[i]);
    os.flush();
    fclose(fp);

    // Read it back to verify
    fp = fopen(filename, "rb");
    FileReadStream is(fp, buffer, sizeof(buffer));

    for (size_t i = 0; i < length_; i++)
        EXPECT_EQ(json_[i], is.get());

    EXPECT_EQ(length_, is.tellg());
    fclose(fp);

    //std::cout << filename << std::endl;
    remove(filename);
}
