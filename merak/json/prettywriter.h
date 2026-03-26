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

#ifndef MERAK_JSON_PRETTYWRITER_H_
#define MERAK_JSON_PRETTYWRITER_H_

#include <merak/json/writer.h>

#ifdef __GNUC__
MERAK_JSON_DIAG_PUSH
MERAK_JSON_DIAG_OFF(effc++)
#endif

#if defined(__clang__)
MERAK_JSON_DIAG_PUSH
MERAK_JSON_DIAG_OFF(c++98-compat)
#endif

namespace merak::json {

    //! Combination of PrettyWriter format flags.
    /*! \see PrettyWriter::SetFormatOptions
     */
    enum PrettyFormatOptions {
        kFormatDefault = 0,         //!< Default pretty formatting.
        kFormatSingleLineArray = 1  //!< Format arrays on a single line.
    };

    //! Writer with indentation and spacing.
    /*!
        \tparam OutputStream Type of output os.
        \tparam SourceEncoding Encoding of source string.
        \tparam TargetEncoding Encoding of output stream.
        \tparam StackAllocator Type of allocator for allocating memory of stack.
    */
    template<typename OutputStream, typename SourceEncoding = UTF8<>, typename TargetEncoding = UTF8<>, typename StackAllocator = CrtAllocator, unsigned writeFlags = kWriteDefaultFlags>
    class PrettyWriter : public Writer<OutputStream, SourceEncoding, TargetEncoding, StackAllocator, writeFlags> {
    public:
        typedef Writer<OutputStream, SourceEncoding, TargetEncoding, StackAllocator, writeFlags> Base;
        typedef typename Base::char_type char_type;

        //! Constructor
        /*! \param os Output stream.
            \param allocator User supplied allocator. If it is null, it will create a private one.
            \param levelDepth Initial capacity of stack.
        */
        explicit PrettyWriter(OutputStream &os, StackAllocator *allocator = 0,
                              size_t levelDepth = Base::kDefaultLevelDepth) :
                Base(os, allocator, levelDepth), indentChar_(' '), indentCharCount_(4),
                formatOptions_(kFormatDefault) {}


        explicit PrettyWriter(StackAllocator *allocator = 0, size_t levelDepth = Base::kDefaultLevelDepth) :
                Base(allocator, levelDepth), indentChar_(' '), indentCharCount_(4), formatOptions_(kFormatDefault) {}


        PrettyWriter(PrettyWriter &&rhs) :
                Base(std::forward<PrettyWriter>(rhs)), indentChar_(rhs.indentChar_),
                indentCharCount_(rhs.indentCharCount_), formatOptions_(rhs.formatOptions_) {}


        //! Set custom indentation.
        /*! \param indentChar       Character for indentation. Must be whitespace character (' ', '\\t', '\\n', '\\r').
            \param indentCharCount  Number of indent characters for each indentation level.
            \note The default indentation is 4 spaces.
        */
        PrettyWriter &SetIndent(char_type indentChar, unsigned indentCharCount) {
            MERAK_JSON_ASSERT(indentChar == ' ' || indentChar == '\t' || indentChar == '\n' || indentChar == '\r');
            indentChar_ = indentChar;
            indentCharCount_ = indentCharCount;
            return *this;
        }

        //! Set pretty writer formatting options.
        /*! \param options Formatting options.
        */
        PrettyWriter &SetFormatOptions(PrettyFormatOptions options) {
            formatOptions_ = options;
            return *this;
        }

        /*! @name Implementation of Handler
            \see Handler
        */
        //@{

        bool emplace_null() {
            PrettyPrefix(kNullType);
            return Base::EndValue(Base::WriteNull());
        }

        bool emplace_bool(bool b) {
            PrettyPrefix(b ? kTrueType : kFalseType);
            return Base::EndValue(Base::write_bool(b));
        }

        bool emplace_int32(int i) {
            PrettyPrefix(kNumberType);
            return Base::EndValue(Base::write_int(i));
        }

        bool emplace_uint32(unsigned u) {
            PrettyPrefix(kNumberType);
            return Base::EndValue(Base::write_uint(u));
        }

        bool emplace_int64(int64_t i64) {
            PrettyPrefix(kNumberType);
            return Base::EndValue(Base::write_int64(i64));
        }

        bool emplace_uint64(uint64_t u64) {
            PrettyPrefix(kNumberType);
            return Base::EndValue(Base::write_uint64(u64));
        }

        bool emplace_double(double d) {
            PrettyPrefix(kNumberType);
            return Base::EndValue(Base::write_double(d));
        }

        bool raw_number(const char_type *str, SizeType length, bool copy = false) {
            MERAK_JSON_ASSERT(str != 0);
            (void) copy;
            PrettyPrefix(kNumberType);
            return Base::EndValue(Base::write_string(str, length));
        }

        bool emplace_string(const char_type *str, SizeType length, bool copy = false) {
            MERAK_JSON_ASSERT(str != 0);
            (void) copy;
            PrettyPrefix(kStringType);
            return Base::EndValue(Base::write_string(str, length));
        }


        bool emplace_string(const std::basic_string<char_type>& str) {
            return emplace_string(str.data(), SizeType(str.size()));
        }


        bool start_object() {
            PrettyPrefix(kObjectType);
            new(Base::level_stack_.template Push<typename Base::Level>()) typename Base::Level(false);
            return Base::WriteStartObject();
        }

        bool emplace_key(const char_type *str, SizeType length, bool copy = false) { return emplace_string(str, length, copy); }


        bool emplace_key(const std::basic_string<char_type>& str) {
            return emplace_key(str.data(), SizeType(str.size()));
        }


        bool end_object(SizeType memberCount = 0) {
            (void) memberCount;
            MERAK_JSON_ASSERT(Base::level_stack_.GetSize() >= sizeof(typename Base::Level)); // not inside an Object
            MERAK_JSON_ASSERT(
                    !Base::level_stack_.template Top<typename Base::Level>()->inArray); // currently inside an Array, not Object
            MERAK_JSON_ASSERT(0 == Base::level_stack_.template Top<typename Base::Level>()->valueCount %
                                   2); // Object has a Key without a Value

            bool empty = Base::level_stack_.template Pop<typename Base::Level>(1)->valueCount == 0;

            if (!empty) {
                Base::os_->put('\n');
                WriteIndent();
            }
            bool ret = Base::EndValue(Base::WriteEndObject());
            (void) ret;
            MERAK_JSON_ASSERT(ret == true);
            if (Base::level_stack_.empty()) // end of json text
                Base::flush();
            return true;
        }

        bool start_array() {
            PrettyPrefix(kArrayType);
            new(Base::level_stack_.template Push<typename Base::Level>()) typename Base::Level(true);
            return Base::WriteStartArray();
        }

        bool end_array(SizeType memberCount = 0) {
            (void) memberCount;
            MERAK_JSON_ASSERT(Base::level_stack_.GetSize() >= sizeof(typename Base::Level));
            MERAK_JSON_ASSERT(Base::level_stack_.template Top<typename Base::Level>()->inArray);
            bool empty = Base::level_stack_.template Pop<typename Base::Level>(1)->valueCount == 0;

            if (!empty && !(formatOptions_ & kFormatSingleLineArray)) {
                Base::os_->put('\n');
                WriteIndent();
            }
            bool ret = Base::EndValue(Base::WriteEndArray());
            (void) ret;
            MERAK_JSON_ASSERT(ret == true);
            if (Base::level_stack_.empty()) // end of json text
                Base::flush();
            return true;
        }

        //@}

        /*! @name Convenience extensions */
        //@{

        //! Simpler but slower overload.
        bool emplace_string(const char_type *str) { return emplace_string(str, internal::StrLen(str)); }

        bool emplace_key(const char_type *str) { return emplace_key(str, internal::StrLen(str)); }

        //@}

        //! Write a raw JSON value.
        /*!
            For user to write a stringified JSON as a value.

            \param json A well-formed JSON value. It should not contain null character within [0, length - 1] range.
            \param length Length of the json.
            \param type Type of the root of json.
            \note When using PrettyWriter::raw_value(), the result json may not be indented correctly.
        */
        bool raw_value(const char_type *json, size_t length, Type type) {
            MERAK_JSON_ASSERT(json != 0);
            PrettyPrefix(type);
            return Base::EndValue(Base::WriteRawValue(json, length));
        }

    protected:
        void PrettyPrefix(Type type) {
            (void) type;
            if (Base::level_stack_.GetSize() != 0) { // this value is not at root
                typename Base::Level *level = Base::level_stack_.template Top<typename Base::Level>();

                if (level->inArray) {
                    if (level->valueCount > 0) {
                        Base::os_->put(','); // add comma if it is not the first element in array
                        if (formatOptions_ & kFormatSingleLineArray)
                            Base::os_->put(' ');
                    }

                    if (!(formatOptions_ & kFormatSingleLineArray)) {
                        Base::os_->put('\n');
                        WriteIndent();
                    }
                } else {  // in object
                    if (level->valueCount > 0) {
                        if (level->valueCount % 2 == 0) {
                            Base::os_->put(',');
                            Base::os_->put('\n');
                        } else {
                            Base::os_->put(':');
                            Base::os_->put(' ');
                        }
                    } else
                        Base::os_->put('\n');

                    if (level->valueCount % 2 == 0)
                        WriteIndent();
                }
                if (!level->inArray && level->valueCount % 2 == 0)
                    MERAK_JSON_ASSERT(type == kStringType);  // if it's in object, then even number should be a name
                level->valueCount++;
            } else {
                MERAK_JSON_ASSERT(!Base::hasRoot_);  // Should only has one and only one root.
                Base::hasRoot_ = true;
            }
        }

        void WriteIndent() {
            size_t count = (Base::level_stack_.GetSize() / sizeof(typename Base::Level)) * indentCharCount_;
            PutN(*Base::os_, static_cast<typename OutputStream::char_type>(indentChar_), count);
        }

        char_type indentChar_;
        unsigned indentCharCount_;
        PrettyFormatOptions formatOptions_;

    private:
        // Prohibit copy constructor & assignment operator.
        PrettyWriter(const PrettyWriter &);

        PrettyWriter &operator=(const PrettyWriter &);
    };

}  // namespace merak::json

#if defined(__clang__)
MERAK_JSON_DIAG_POP
#endif

#ifdef __GNUC__
MERAK_JSON_DIAG_POP
#endif

#endif // MERAK_JSON_PRETTYWRITER_H_
