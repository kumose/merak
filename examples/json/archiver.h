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

#ifndef ARCHIVER_H_
#define ARCHIVER_H_

#include <cstddef>
#include <string>

/**
\class Archiver
\brief Archiver concept

Archiver can be a reader or writer for serialization or deserialization respectively.

class Archiver {
public:
    /// \returns true if the archiver is in normal state. false if it has errors.
    operator bool() const;

    /// Starts an object
    Archiver& start_object();
    
    /// After calling start_object(), assign a member with a name
    Archiver& Member(const char* name);

    /// After calling start_object(), check if a member presents
    bool has_member(const char* name) const;

    /// Ends an object
    Archiver& end_object();

    /// Starts an array
    /// \param size If Archiver::IsReader is true, the size of array is written.
    Archiver& start_array(size_t* size = 0);

    /// Ends an array
    Archiver& end_array();

    /// Read/Write primitive types.
    Archiver& operator&(bool& b);
    Archiver& operator&(unsigned& u);
    Archiver& operator&(int& i);
    Archiver& operator&(double& d);
    Archiver& operator&(std::string& s);

    /// Write primitive types.
    Archiver& SetNull();

    //! Whether it is a reader.
    static const bool IsReader;

    //! Whether it is a writer.
    static const bool IsWriter;
};
*/

/// Represents a JSON reader which implements Archiver concept.
class JsonReader {
public:
    /// Constructor.
    /**
        \param json A non-const source json string for in-situ parsing.
        \note in-situ means the source JSON string will be modified after parsing.
    */
    JsonReader(const char* json);

    /// Destructor.
    ~JsonReader();

    // Archive concept

    operator bool() const { return !mError; }

    JsonReader& start_object();
    JsonReader& Member(const char* name);
    bool has_member(const char* name) const;
    JsonReader& end_object();

    JsonReader& start_array(size_t* size = 0);
    JsonReader& end_array();

    JsonReader& operator&(bool& b);
    JsonReader& operator&(unsigned& u);
    JsonReader& operator&(int& i);
    JsonReader& operator&(double& d);
    JsonReader& operator&(std::string& s);

    JsonReader& SetNull();

    static const bool IsReader = true;
    static const bool IsWriter = !IsReader;

private:
    JsonReader(const JsonReader&);
    JsonReader& operator=(const JsonReader&);

    void Next();

    // PIMPL
    void* mDocument;              ///< DOM result of parsing.
    void* mStack;                 ///< Stack for iterating the DOM
    bool mError;                  ///< Whether an error has occurred.
};

class JsonWriter {
public:
    /// Constructor.
    JsonWriter();

    /// Destructor.
    ~JsonWriter();

    /// Obtains the serialized JSON string.
    const char* get_string() const;

    // Archive concept

    operator bool() const { return true; }

    JsonWriter& start_object();
    JsonWriter& Member(const char* name);
    bool has_member(const char* name) const;
    JsonWriter& end_object();

    JsonWriter& start_array(size_t* size = 0);
    JsonWriter& end_array();

    JsonWriter& operator&(bool& b);
    JsonWriter& operator&(unsigned& u);
    JsonWriter& operator&(int& i);
    JsonWriter& operator&(double& d);
    JsonWriter& operator&(std::string& s);
    JsonWriter& SetNull();

    static const bool IsReader = false;
    static const bool IsWriter = !IsReader;

private:
    JsonWriter(const JsonWriter&);
    JsonWriter& operator=(const JsonWriter&);

    // PIMPL idiom
    void* mWriter;      ///< JSON writer.
    void* mStream;      ///< Stream buffer.
};

#endif // ARCHIVER_H__
