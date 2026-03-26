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
// Example of parsing JSON to document by parts.

// Using C++11 threads
// Temporarily disable for clang (older version) due to incompatibility with libstdc++
#if (__cplusplus >= 201103L || (defined(_MSC_VER) && _MSC_VER >= 1700)) && !defined(__clang__)

#include <merak/json.h>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>

using namespace merak::json;

template<unsigned parseFlags = kParseDefaultFlags>
class AsyncDocumentParser {
public:
    AsyncDocumentParser(Document& d)
        : stream_(*this)
        , d_(d)
        , parseThread_()
        , mutex_()
        , notEmpty_()
        , finish_()
        , completed_()
    {
        // Create and execute thread after all member variables are initialized.
        parseThread_ = std::thread(&AsyncDocumentParser::parse, this);
    }

    ~AsyncDocumentParser() {
        if (!parseThread_.joinable())
            return;

        {        
            std::unique_lock<std::mutex> lock(mutex_);

            // Wait until the buffer is read up (or parsing is completed)
            while (!stream_.empty() && !completed_)
                finish_.wait(lock);

            // Automatically append '\0' as the terminator in the stream.
            static const char terminator[] = "";
            stream_.src_ = terminator;
            stream_.end_ = terminator + 1;
            notEmpty_.notify_one(); // unblock the AsyncStringStream
        }

        parseThread_.join();
    }

    void ParsePart(const char* buffer, size_t length) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        // Wait until the buffer is read up (or parsing is completed)
        while (!stream_.empty() && !completed_)
            finish_.wait(lock);

        // Stop further parsing if the parsing process is completed.
        if (completed_)
            return;

        // Set the buffer to stream and unblock the AsyncStringStream
        stream_.src_ = buffer;
        stream_.end_ = buffer + length;
        notEmpty_.notify_one();
    }

private:
    void parse() {
        d_.parse_stream<parseFlags>(stream_);

        // The stream may not be fully read, notify finish anyway to unblock ParsePart()
        std::unique_lock<std::mutex> lock(mutex_);
        completed_ = true;      // Parsing process is completed
        finish_.notify_one();   // Unblock ParsePart() or destructor if they are waiting.
    }

    struct AsyncStringStream {
        typedef char char_type;

        AsyncStringStream(AsyncDocumentParser& parser) : parser_(parser), src_(), end_(), count_() {}

        char peek() const {
            std::unique_lock<std::mutex> lock(parser_.mutex_);

            // If nothing in stream, block to wait.
            while (empty())
                parser_.notEmpty_.wait(lock);

            return *src_;
        }

        char get() {
            std::unique_lock<std::mutex> lock(parser_.mutex_);

            // If nothing in stream, block to wait.
            while (empty())
                parser_.notEmpty_.wait(lock);

            count_++;
            char c = *src_++;

            // If all stream is read up, notify that the stream is finish.
            if (empty())
                parser_.finish_.notify_one();

            return c;
        }

        size_t tellg() const { return count_; }


        void put(char) {}
        AsyncStringStream &flush() {
            return *this;
        }

        bool empty() const { return src_ == end_; }

        AsyncDocumentParser& parser_;
        const char* src_;     //!< Current read position.
        const char* end_;     //!< end of buffer
        size_t count_;        //!< Number of characters taken so far.
    };

    AsyncStringStream stream_;
    Document& d_;
    std::thread parseThread_;
    std::mutex mutex_;
    std::condition_variable notEmpty_;
    std::condition_variable finish_;
    bool completed_;
};

int main() {
    Document d;

    {
        AsyncDocumentParser<> parser(d);

        const char json1[] = " { \"hello\" : \"world\", \"t\" : tr";
        //const char json1[] = " { \"hello\" : \"world\", \"t\" : trX"; // For test parsing error
        const char json2[] = "ue, \"f\" : false, \"n\": null, \"i\":123, \"pi\": 3.14";
        const char json3[] = "16, \"a\":[1, 2, 3, 4] } ";

        parser.ParsePart(json1, sizeof(json1) - 1);
        parser.ParsePart(json2, sizeof(json2) - 1);
        parser.ParsePart(json3, sizeof(json3) - 1);
    }

    if (d.has_parse_error()) {
        std::cout << "Error at offset " << d.get_error_offset() << ": " << get_parse_error_en(d.get_parse_error()) << std::endl;
        return EXIT_FAILURE;
    }
    
    // Stringify the JSON to cout
    StdOutputStream os(std::cout);
    Writer<StdOutputStream> writer(os);
    d.accept(writer);
    std::cout << std::endl;

    std::cout<<std::char_traits<char>::eof()<<std::endl;
    std::cout<<std::char_traits<char16_t>::eof()<<std::endl;
    std::cout<<std::char_traits<char32_t>::eof()<<std::endl;
    std::cout<<std::char_traits<wchar_t>::eof()<<std::endl;
    return EXIT_SUCCESS;
}

#else // Not supporting C++11 

#include <iostream>
int main() {
    std::cout << "This example requires C++11 compiler" << std::endl;
}

#endif
