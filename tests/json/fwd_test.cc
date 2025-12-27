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

// Using forward declared types here.

#include <merak/json/fwd.h>

#ifdef __GNUC__
MERAK_JSON_DIAG_PUSH
MERAK_JSON_DIAG_OFF(effc++)
#endif

using namespace merak::json;

struct Foo {
    Foo();
    ~Foo();

    // encodings.h
    UTF8<char>* utf8;
    UTF16<wchar_t>* utf16;
    UTF16BE<wchar_t>* utf16be;
    UTF16LE<wchar_t>* utf16le;
    UTF32<unsigned>* utf32;
    UTF32BE<unsigned>* utf32be;
    UTF32LE<unsigned>* utf32le;
    ASCII<char>* ascii;
    AutoUTF<unsigned>* autoutf;
    Transcoder<UTF8<char>, UTF8<char> >* transcoder;

    // allocators.h
    CrtAllocator* crtallocator;
    MemoryPoolAllocator<CrtAllocator>* memorypoolallocator;

    // stream.h
    StringStream* stringstream;
    InsituStringStream* insitustringstream;

    // stringbuffer.h
    StringBuffer* stringbuffer;

    // // filereadstream.h
    // FileReadStream* filereadstream;

    // // filewritestream.h
    // FileWriteStream* filewritestream;

    // memorybuffer.h
    MemoryBuffer* memorybuffer;

    // memorystream.h
    MemoryStream* memorystream;

    // reader.h
    BaseReaderHandler<UTF8<char>, void>* basereaderhandler;
    Reader* reader;

    // writer.h
    Writer<StringBuffer, UTF8<char>, UTF8<char>, CrtAllocator, 0>* writer;

    // prettywriter.h
    PrettyWriter<StringBuffer, UTF8<char>, UTF8<char>, CrtAllocator, 0>* prettywriter;

    // document.h
    Value* value;
    Document* document;

    // pointer.h
    Pointer* pointer;

    // schema.h
    SchemaDocument* schemadocument;
    SchemaValidator* schemavalidator;

    // char buffer[16];
};

// Using type definitions here.

#include <merak/json/stringbuffer.h>
#include <merak/json/filereadstream.h>
#include <merak/json/filewritestream.h>
#include <merak/json/memorybuffer.h>
#include <merak/json/memorystream.h>
#include <merak/json/document.h> // -> reader.h
#include <merak/json/writer.h>
#include <merak/json/prettywriter.h>
#include <merak/json/schema.h>   // -> pointer.h

typedef Transcoder<UTF8<>, UTF8<> > TranscoderUtf8ToUtf8;
typedef BaseReaderHandler<UTF8<>, void> BaseReaderHandlerUtf8Void;

Foo::Foo() : 
    // encodings.h
    utf8(MERAK_JSON_NEW(UTF8<>)),
    utf16(MERAK_JSON_NEW(UTF16<>)),
    utf16be(MERAK_JSON_NEW(UTF16BE<>)),
    utf16le(MERAK_JSON_NEW(UTF16LE<>)),
    utf32(MERAK_JSON_NEW(UTF32<>)),
    utf32be(MERAK_JSON_NEW(UTF32BE<>)),
    utf32le(MERAK_JSON_NEW(UTF32LE<>)),
    ascii(MERAK_JSON_NEW(ASCII<>)),
    autoutf(MERAK_JSON_NEW(AutoUTF<unsigned>)),
    transcoder(MERAK_JSON_NEW(TranscoderUtf8ToUtf8)),

    // allocators.h
    crtallocator(MERAK_JSON_NEW(CrtAllocator)),
    memorypoolallocator(MERAK_JSON_NEW(MemoryPoolAllocator<>)),

    // stream.h
    stringstream(MERAK_JSON_NEW(StringStream)(NULL)),
    insitustringstream(MERAK_JSON_NEW(InsituStringStream)(NULL)),

    // stringbuffer.h
    stringbuffer(MERAK_JSON_NEW(StringBuffer)),

    // // filereadstream.h
    // filereadstream(MERAK_JSON_NEW(FileReadStream)(stdout, buffer, sizeof(buffer))),

    // // filewritestream.h
    // filewritestream(MERAK_JSON_NEW(FileWriteStream)(stdout, buffer, sizeof(buffer))),

    // memorybuffer.h
    memorybuffer(MERAK_JSON_NEW(MemoryBuffer)),

    // memorystream.h
    memorystream(MERAK_JSON_NEW(MemoryStream)(NULL, 0)),

    // reader.h
    basereaderhandler(MERAK_JSON_NEW(BaseReaderHandlerUtf8Void)),
    reader(MERAK_JSON_NEW(Reader)),

    // writer.h
    writer(MERAK_JSON_NEW(Writer<StringBuffer>)),

    // prettywriter.h
    prettywriter(MERAK_JSON_NEW(PrettyWriter<StringBuffer>)),

    // document.h
    value(MERAK_JSON_NEW(Value)),
    document(MERAK_JSON_NEW(Document)),

    // pointer.h
    pointer(MERAK_JSON_NEW(Pointer)),

    // schema.h
    schemadocument(MERAK_JSON_NEW(SchemaDocument)(*document)),
    schemavalidator(MERAK_JSON_NEW(SchemaValidator)(*schemadocument))
{

}

Foo::~Foo() {
    // encodings.h
    MERAK_JSON_DELETE(utf8);
    MERAK_JSON_DELETE(utf16);
    MERAK_JSON_DELETE(utf16be);
    MERAK_JSON_DELETE(utf16le);
    MERAK_JSON_DELETE(utf32);
    MERAK_JSON_DELETE(utf32be);
    MERAK_JSON_DELETE(utf32le);
    MERAK_JSON_DELETE(ascii);
    MERAK_JSON_DELETE(autoutf);
    MERAK_JSON_DELETE(transcoder);

    // allocators.h
    MERAK_JSON_DELETE(crtallocator);
    MERAK_JSON_DELETE(memorypoolallocator);

    // stream.h
    MERAK_JSON_DELETE(stringstream);
    MERAK_JSON_DELETE(insitustringstream);

    // stringbuffer.h
    MERAK_JSON_DELETE(stringbuffer);

    // // filereadstream.h
    // MERAK_JSON_DELETE(filereadstream);

    // // filewritestream.h
    // MERAK_JSON_DELETE(filewritestream);

    // memorybuffer.h
    MERAK_JSON_DELETE(memorybuffer);

    // memorystream.h
    MERAK_JSON_DELETE(memorystream);

    // reader.h
    MERAK_JSON_DELETE(basereaderhandler);
    MERAK_JSON_DELETE(reader);

    // writer.h
    MERAK_JSON_DELETE(writer);

    // prettywriter.h
    MERAK_JSON_DELETE(prettywriter);

    // document.h
    MERAK_JSON_DELETE(value);
    MERAK_JSON_DELETE(document);

    // pointer.h
    MERAK_JSON_DELETE(pointer);

    // schema.h
    MERAK_JSON_DELETE(schemadocument);
    MERAK_JSON_DELETE(schemavalidator);
}

TEST(Fwd, Fwd) {
    Foo f;
}

#ifdef __GNUC__
MERAK_JSON_DIAG_POP
#endif
