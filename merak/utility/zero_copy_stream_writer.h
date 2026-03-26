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
#pragma once

#include <google/protobuf/io/zero_copy_stream.h> // ZeroCopyOutputStream
#include <iostream>

//class IOBufAsZeroCopyOutputStream
//    : public google::protobuf::io::ZeroCopyOutputStream {
//public:
//    explicit IOBufAsZeroCopyOutputStream(IOBuf*);
//
//    // Interfaces of ZeroCopyOutputStream
//    bool Next(void** data, int* size);
//    void BackUp(int count);
//    google::protobuf::int64 ByteCount() const;
//
//private:
//    IOBuf* _buf;
//    size_t _initial_length;
//};

namespace merak {

    class ZeroCopyStreamWriter {
    public:
        typedef char char_type;

        ZeroCopyStreamWriter(google::protobuf::io::ZeroCopyOutputStream *stream)
                : _stream(stream), _data(nullptr),
                  _cursor(nullptr), _data_size(0) {
        }

        ~ZeroCopyStreamWriter() {
            if (_stream && _data) {
                _stream->BackUp(RemainSize());
            }
            _stream = nullptr;
        }

        void put(char c) {
            if (__builtin_expect(AcquireNextBuf(), 1)) {
                *_cursor = c;
                ++_cursor;
            }
        }

        void PutN(char c, size_t n) {
            while (AcquireNextBuf() && n > 0) {
                size_t remain_size = RemainSize();
                size_t to_write = n > remain_size ? remain_size : n;
                memset(_cursor, c, to_write);
                _cursor += to_write;
                n -= to_write;
            }
        }

        void write(const char *str, size_t length) {
            while (AcquireNextBuf() && length > 0) {
                size_t remain_size = RemainSize();
                size_t to_write = length > remain_size ? remain_size : length;
                memcpy(_cursor, str, to_write);
                _cursor += to_write;
                str += to_write;
                length -= to_write;
            }
        }

        ZeroCopyStreamWriter& flush() {
            return *this;
        }

    private:
        bool AcquireNextBuf() {
            if (__builtin_expect(!_stream, 0)) {
                return false;
            }
            if (_data == nullptr || _cursor == _data + _data_size) {
                if (!_stream->Next((void **) &_data, &_data_size)) {
                    return false;
                }
                _cursor = _data;
            }
            return true;
        }

        size_t RemainSize() {
            return _data_size - (_cursor - _data);
        }

        google::protobuf::io::ZeroCopyOutputStream *_stream;
        char *_data;
        char *_cursor;
        int _data_size;
    };

}  // namespace merak
