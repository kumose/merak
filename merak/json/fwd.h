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

#ifndef MERAK_JSON_FWD_H_
#define MERAK_JSON_FWD_H_

#include <merak/json/json_internal.h>

namespace merak::json {

// encodings.h

template<typename CharType> struct UTF8;
template<typename CharType> struct UTF16;
template<typename CharType> struct UTF16BE;
template<typename CharType> struct UTF16LE;
template<typename CharType> struct UTF32;
template<typename CharType> struct UTF32BE;
template<typename CharType> struct UTF32LE;
template<typename CharType> struct ASCII;
template<typename CharType> struct AutoUTF;

template<typename SourceEncoding, typename TargetEncoding>
struct Transcoder;

// allocators.h

class CrtAllocator;

template <typename BaseAllocator>
class MemoryPoolAllocator;

// stream.h

template <typename Encoding>
struct GenericStringStream;

typedef GenericStringStream<UTF8<char> > StringStream;

template <typename Encoding>
struct GenericInsituStringStream;

typedef GenericInsituStringStream<UTF8<char> > InsituStringStream;

// stringbuffer.h

template <typename Encoding, typename Allocator>
class GenericStringBuffer;

typedef GenericStringBuffer<UTF8<char>, CrtAllocator> StringBuffer;

// filereadstream.h

class FileReadStream;

// filewritestream.h

class FileWriteStream;

// memorybuffer.h

template <typename Allocator>
struct GenericMemoryBuffer;

typedef GenericMemoryBuffer<CrtAllocator> MemoryBuffer;

// memorystream.h

struct MemoryStream;

// reader.h

template<typename Encoding, typename Derived>
struct BaseReaderHandler;

template <typename SourceEncoding, typename TargetEncoding, typename StackAllocator>
class GenericReader;

typedef GenericReader<UTF8<char>, UTF8<char>, CrtAllocator> Reader;

// writer.h

template<typename OutputStream, typename SourceEncoding, typename TargetEncoding, typename StackAllocator, unsigned writeFlags>
class Writer;

// prettywriter.h

template<typename OutputStream, typename SourceEncoding, typename TargetEncoding, typename StackAllocator, unsigned writeFlags>
class PrettyWriter;

// document.h

template <typename Encoding, typename Allocator> 
class GenericMember;

template <bool Const, typename Encoding, typename Allocator>
class GenericMemberIterator;

template<typename CharType>
struct GenericStringRef;

template <typename Encoding, typename Allocator> 
class GenericValue;

typedef GenericValue<UTF8<char>, MemoryPoolAllocator<CrtAllocator> > Value;

template <typename Encoding, typename Allocator, typename StackAllocator>
class GenericDocument;

typedef GenericDocument<UTF8<char>, MemoryPoolAllocator<CrtAllocator>, CrtAllocator> Document;

// pointer.h

template <typename ValueType, typename Allocator>
class GenericPointer;

typedef GenericPointer<Value, CrtAllocator> Pointer;

// schema.h

template <typename SchemaDocumentType>
class IGenericRemoteSchemaDocumentProvider;

template <typename ValueT, typename Allocator>
class GenericSchemaDocument;

typedef GenericSchemaDocument<Value, CrtAllocator> SchemaDocument;
typedef IGenericRemoteSchemaDocumentProvider<SchemaDocument> IRemoteSchemaDocumentProvider;

template <
    typename SchemaDocumentType,
    typename OutputHandler,
    typename StateAllocator>
class GenericSchemaValidator;

typedef GenericSchemaValidator<SchemaDocument, BaseReaderHandler<UTF8<char>, void>, CrtAllocator> SchemaValidator;

}  // namespace merak::json

#endif // MERAK_JSON_RAPIDJSONFWD_H_
