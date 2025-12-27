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

#ifndef MERAK_JSON_ERROR_ERROR_H_
#define MERAK_JSON_ERROR_ERROR_H_

#include <merak/json/json_internal.h>

#ifdef __clang__
MERAK_JSON_DIAG_PUSH
MERAK_JSON_DIAG_OFF(padded)
#endif
/*! \file error.h */

/*! \defgroup MERAK_JSON_ERRORS RapidJSON error handling */

///////////////////////////////////////////////////////////////////////////////
// MERAK_JSON_ERROR_CHARTYPE

//! Character type of error messages.
/*! \ingroup MERAK_JSON_ERRORS
    The default character type is \c char.
    On Windows, user can define this macro as \c TCHAR for supporting both
    unicode/non-unicode settings.
*/
#ifndef MERAK_JSON_ERROR_CHARTYPE
#define MERAK_JSON_ERROR_CHARTYPE char
#endif

///////////////////////////////////////////////////////////////////////////////
// MERAK_JSON_ERROR_STRING

//! Macro for converting string literal to \ref MERAK_JSON_ERROR_CHARTYPE[].
/*! \ingroup MERAK_JSON_ERRORS
    By default this conversion macro does nothing.
    On Windows, user can define this macro as \c _T(x) for supporting both
    unicode/non-unicode settings.
*/
#ifndef MERAK_JSON_ERROR_STRING
#define MERAK_JSON_ERROR_STRING(x) x
#endif

namespace merak::json {

    ///////////////////////////////////////////////////////////////////////////////
    // ParseErrorCode

    //! Error code of parsing.
    /*! \ingroup MERAK_JSON_ERRORS
        \see GenericReader::parse, GenericReader::GetParseErrorCode
    */
    enum ParseErrorCode {
        kParseErrorNone = 0,                        //!< No error.

        kParseErrorDocumentEmpty,                   //!< The document is empty.
        kParseErrorDocumentRootNotSingular,         //!< The document root must not follow by other values.

        kParseErrorValueInvalid,                    //!< Invalid value.

        kParseErrorObjectMissName,                  //!< Missing a name for object member.
        kParseErrorObjectMissColon,                 //!< Missing a colon after a name of object member.
        kParseErrorObjectMissCommaOrCurlyBracket,   //!< Missing a comma or '}' after an object member.

        kParseErrorArrayMissCommaOrSquareBracket,   //!< Missing a comma or ']' after an array element.

        kParseErrorStringUnicodeEscapeInvalidHex,   //!< Incorrect hex digit after \\u escape in string.
        kParseErrorStringUnicodeSurrogateInvalid,   //!< The surrogate pair in string is invalid.
        kParseErrorStringEscapeInvalid,             //!< Invalid escape character in string.
        kParseErrorStringMissQuotationMark,         //!< Missing a closing quotation mark in string.
        kParseErrorStringInvalidEncoding,           //!< Invalid encoding in string.

        kParseErrorNumberTooBig,                    //!< Number too big to be stored in double.
        kParseErrorNumberMissFraction,              //!< Miss fraction part in number.
        kParseErrorNumberMissExponent,              //!< Miss exponent in number.

        kParseErrorTermination,                     //!< Parsing was terminated.
        kParseErrorUnspecificSyntaxError            //!< Unspecific syntax error.
    };

    //! Result of parsing (wraps ParseErrorCode)
    /*!
        \ingroup MERAK_JSON_ERRORS
        \code
            Document doc;
            ParseResult ok = doc.parse("[42]");
            if (!ok) {
                fprintf(stderr, "JSON parse error: %s (%u)",
                        get_parse_error_en(ok.Code()), ok.Offset());
                exit(EXIT_FAILURE);
            }
        \endcode
        \see GenericReader::parse, GenericDocument::parse
    */
    struct ParseResult {
        //!! Unspecified boolean type
        typedef bool (ParseResult::*BooleanType)() const;

    public:
        //! Default constructor, no error.
        ParseResult() : code_(kParseErrorNone), offset_(0) {}

        //! Constructor to set an error.
        ParseResult(ParseErrorCode code, size_t offset) : code_(code), offset_(offset) {}

        //! Get the error code.
        ParseErrorCode Code() const { return code_; }

        //! Get the error offset, if \ref IsError(), 0 otherwise.
        size_t Offset() const { return offset_; }

        //! Explicit conversion to \c bool, returns \c true, iff !\ref IsError().
        operator BooleanType() const { return !IsError() ? &ParseResult::IsError : NULL; }

        //! Whether the result is an error.
        bool IsError() const { return code_ != kParseErrorNone; }

        bool operator==(const ParseResult &that) const { return code_ == that.code_; }

        bool operator==(ParseErrorCode code) const { return code_ == code; }

        friend bool operator==(ParseErrorCode code, const ParseResult &err) { return code == err.code_; }

        bool operator!=(const ParseResult &that) const { return !(*this == that); }

        bool operator!=(ParseErrorCode code) const { return !(*this == code); }

        friend bool operator!=(ParseErrorCode code, const ParseResult &err) { return err != code; }

        //! Reset error code.
        void clear() { set(kParseErrorNone); }

        //! Update error code and offset.
        void set(ParseErrorCode code, size_t offset = 0) {
            code_ = code;
            offset_ = offset;
        }

    private:
        ParseErrorCode code_;
        size_t offset_;
    };

    //! Function pointer type of get_parse_error().
    /*! \ingroup MERAK_JSON_ERRORS

        This is the prototype for \c GetParseError_X(), where \c X is a locale.
        User can dynamically change locale in runtime, e.g.:
    \code
        GetParseErrorFunc get_parse_error = get_parse_error_en; // or whatever
        const MERAK_JSON_ERROR_CHARTYPE* s = get_parse_error(document.GetParseErrorCode());
    \endcode
    */
    typedef const MERAK_JSON_ERROR_CHARTYPE *(*GetParseErrorFunc)(ParseErrorCode);

    ///////////////////////////////////////////////////////////////////////////////
    // ValidateErrorCode

    //! Error codes when validating.
    /*! \ingroup MERAK_JSON_ERRORS
        \see GenericSchemaValidator
    */
    enum ValidateErrorCode {
        kValidateErrors = -1,                   //!< Top level error code when kValidateContinueOnErrorsFlag set.
        kValidateErrorNone = 0,                    //!< No error.

        kValidateErrorMultipleOf,                  //!< Number is not a multiple of the 'multipleOf' value.
        kValidateErrorMaximum,                     //!< Number is greater than the 'maximum' value.
        kValidateErrorExclusiveMaximum,            //!< Number is greater than or equal to the 'maximum' value.
        kValidateErrorMinimum,                     //!< Number is less than the 'minimum' value.
        kValidateErrorExclusiveMinimum,            //!< Number is less than or equal to the 'minimum' value.

        kValidateErrorMaxLength,                   //!< String is longer than the 'maxLength' value.
        kValidateErrorMinLength,                   //!< String is longer than the 'maxLength' value.
        kValidateErrorPattern,                     //!< String does not match the 'pattern' regular expression.

        kValidateErrorMaxItems,                    //!< Array is longer than the 'maxItems' value.
        kValidateErrorMinItems,                    //!< Array is shorter than the 'minItems' value.
        kValidateErrorUniqueItems,                 //!< Array has duplicate items but 'uniqueItems' is true.
        kValidateErrorAdditionalItems,             //!< Array has additional items that are not allowed by the schema.

        kValidateErrorMaxProperties,               //!< Object has more members than 'maxProperties' value.
        kValidateErrorMinProperties,               //!< Object has less members than 'minProperties' value.
        kValidateErrorRequired,                    //!< Object is missing one or more members required by the schema.
        kValidateErrorAdditionalProperties,        //!< Object has additional members that are not allowed by the schema.
        kValidateErrorPatternProperties,           //!< See other errors.
        kValidateErrorDependencies,                //!< Object has missing property or schema dependencies.

        kValidateErrorEnum,                        //!< Property has a value that is not one of its allowed enumerated values.
        kValidateErrorType,                        //!< Property has a type that is not allowed by the schema.

        kValidateErrorOneOf,                       //!< Property did not match any of the sub-schemas specified by 'oneOf'.
        kValidateErrorOneOfMatch,                  //!< Property matched more than one of the sub-schemas specified by 'oneOf'.
        kValidateErrorAllOf,                       //!< Property did not match all of the sub-schemas specified by 'allOf'.
        kValidateErrorAnyOf,                       //!< Property did not match any of the sub-schemas specified by 'anyOf'.
        kValidateErrorNot,                         //!< Property matched the sub-schema specified by 'not'.

        kValidateErrorReadOnly,                    //!< Property is read-only but has been provided when validation is for writing
        kValidateErrorWriteOnly                    //!< Property is write-only but has been provided when validation is for reading
    };

    //! Function pointer type of GetValidateError().
    /*! \ingroup MERAK_JSON_ERRORS

        This is the prototype for \c GetValidateError_X(), where \c X is a locale.
        User can dynamically change locale in runtime, e.g.:
    \code
        GetValidateErrorFunc GetValidateError = GetValidateError_En; // or whatever
        const MERAK_JSON_ERROR_CHARTYPE* s = GetValidateError(validator.GetInvalidSchemaCode());
    \endcode
    */
    typedef const MERAK_JSON_ERROR_CHARTYPE *(*GetValidateErrorFunc)(ValidateErrorCode);

    ///////////////////////////////////////////////////////////////////////////////
    // SchemaErrorCode

    //! Error codes when validating.
    /*! \ingroup MERAK_JSON_ERRORS
        \see GenericSchemaValidator
    */
    enum SchemaErrorCode {
        kSchemaErrorNone = 0,                      //!< No error.

        kSchemaErrorStartUnknown,                  //!< Pointer to start of schema does not resolve to a location in the document
        kSchemaErrorRefPlainName,                  //!< $ref fragment must be a JSON pointer
        kSchemaErrorRefInvalid,                    //!< $ref must not be an empty string
        kSchemaErrorRefPointerInvalid,             //!< $ref fragment is not a valid JSON pointer at offset
        kSchemaErrorRefUnknown,                    //!< $ref does not resolve to a location in the target document
        kSchemaErrorRefCyclical,                   //!< $ref is cyclical
        kSchemaErrorRefNoRemoteProvider,           //!< $ref is remote but there is no remote provider
        kSchemaErrorRefNoRemoteSchema,             //!< $ref is remote but the remote provider did not return a schema
        kSchemaErrorRegexInvalid,                  //!< Invalid regular expression in 'pattern' or 'patternProperties'
        kSchemaErrorSpecUnknown,                   //!< JSON schema draft or OpenAPI version is not recognized
        kSchemaErrorSpecUnsupported,               //!< JSON schema draft or OpenAPI version is not supported
        kSchemaErrorSpecIllegal,                   //!< Both JSON schema draft and OpenAPI version found in document
        kSchemaErrorReadOnlyAndWriteOnly           //!< Property must not be both 'readOnly' and 'writeOnly'
    };

    //! Function pointer type of GetSchemaError().
    /*! \ingroup MERAK_JSON_ERRORS

        This is the prototype for \c GetSchemaError_X(), where \c X is a locale.
        User can dynamically change locale in runtime, e.g.:
    \code
        GetSchemaErrorFunc GetSchemaError = GetSchemaError_En; // or whatever
        const MERAK_JSON_ERROR_CHARTYPE* s = GetSchemaError(validator.GetInvalidSchemaCode());
    \endcode
    */
    typedef const MERAK_JSON_ERROR_CHARTYPE *(*GetSchemaErrorFunc)(SchemaErrorCode);

    ///////////////////////////////////////////////////////////////////////////////
    // PointerParseErrorCode

    //! Error code of JSON pointer parsing.
    /*! \ingroup MERAK_JSON_ERRORS
        \see GenericPointer::GenericPointer, GenericPointer::GetParseErrorCode
    */
    enum PointerParseErrorCode {
        kPointerParseErrorNone = 0,                     //!< The parse is successful

        kPointerParseErrorTokenMustBeginWithSolidus,    //!< A token must begin with a '/'
        kPointerParseErrorInvalidEscape,                //!< Invalid escape
        kPointerParseErrorInvalidPercentEncoding,       //!< Invalid percent encoding in URI fragment
        kPointerParseErrorCharacterMustPercentEncode    //!< A character must percent encoded in URI fragment
    };

    //! Function pointer type of GetPointerParseError().
    /*! \ingroup MERAK_JSON_ERRORS

        This is the prototype for \c GetPointerParseError_X(), where \c X is a locale.
        User can dynamically change locale in runtime, e.g.:
    \code
        GetPointerParseErrorFunc GetPointerParseError = GetPointerParseError_En; // or whatever
        const MERAK_JSON_ERROR_CHARTYPE* s = GetPointerParseError(pointer.GetParseErrorCode());
    \endcode
    */
    typedef const MERAK_JSON_ERROR_CHARTYPE *(*GetPointerParseErrorFunc)(PointerParseErrorCode);


}  // namespace merak::json

#ifdef __clang__
MERAK_JSON_DIAG_POP
#endif

#endif // MERAK_JSON_ERROR_ERROR_H_
