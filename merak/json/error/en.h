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

#ifndef MERAK_JSON_ERROR_EN_H_
#define MERAK_JSON_ERROR_EN_H_


#include <merak/json/error/error.h>
#ifdef __clang__
MERAK_JSON_DIAG_PUSH
MERAK_JSON_DIAG_OFF(switch-enum)
MERAK_JSON_DIAG_OFF(covered-switch-default)
#endif

namespace merak::json {

//! Maps error code of parsing into error message.
/*!
    \ingroup MERAK_JSON_ERRORS
    \param parseErrorCode Error code obtained in parsing.
    \return the error message.
    \note User can make a copy of this function for localization.
        Using switch-case is safer for future modification of error codes.
*/
inline const MERAK_JSON_ERROR_CHARTYPE* get_parse_error_en(ParseErrorCode parseErrorCode) {
    switch (parseErrorCode) {
        case kParseErrorNone:                           return MERAK_JSON_ERROR_STRING("No error.");

        case kParseErrorDocumentEmpty:                  return MERAK_JSON_ERROR_STRING("The document is empty.");
        case kParseErrorDocumentRootNotSingular:        return MERAK_JSON_ERROR_STRING("The document root must not be followed by other values.");

        case kParseErrorValueInvalid:                   return MERAK_JSON_ERROR_STRING("Invalid value.");

        case kParseErrorObjectMissName:                 return MERAK_JSON_ERROR_STRING("Missing a name for object member.");
        case kParseErrorObjectMissColon:                return MERAK_JSON_ERROR_STRING("Missing a colon after a name of object member.");
        case kParseErrorObjectMissCommaOrCurlyBracket:  return MERAK_JSON_ERROR_STRING("Missing a comma or '}' after an object member.");

        case kParseErrorArrayMissCommaOrSquareBracket:  return MERAK_JSON_ERROR_STRING("Missing a comma or ']' after an array element.");

        case kParseErrorStringUnicodeEscapeInvalidHex:  return MERAK_JSON_ERROR_STRING("Incorrect hex digit after \\u escape in string.");
        case kParseErrorStringUnicodeSurrogateInvalid:  return MERAK_JSON_ERROR_STRING("The surrogate pair in string is invalid.");
        case kParseErrorStringEscapeInvalid:            return MERAK_JSON_ERROR_STRING("Invalid escape character in string.");
        case kParseErrorStringMissQuotationMark:        return MERAK_JSON_ERROR_STRING("Missing a closing quotation mark in string.");
        case kParseErrorStringInvalidEncoding:          return MERAK_JSON_ERROR_STRING("Invalid encoding in string.");

        case kParseErrorNumberTooBig:                   return MERAK_JSON_ERROR_STRING("Number too big to be stored in double.");
        case kParseErrorNumberMissFraction:             return MERAK_JSON_ERROR_STRING("Miss fraction part in number.");
        case kParseErrorNumberMissExponent:             return MERAK_JSON_ERROR_STRING("Miss exponent in number.");

        case kParseErrorTermination:                    return MERAK_JSON_ERROR_STRING("Terminate parsing due to Handler error.");
        case kParseErrorUnspecificSyntaxError:          return MERAK_JSON_ERROR_STRING("Unspecific syntax error.");

        default:                                        return MERAK_JSON_ERROR_STRING("Unknown error.");
    }
}

//! Maps error code of validation into error message.
/*!
    \ingroup MERAK_JSON_ERRORS
    \param validateErrorCode Error code obtained from validator.
    \return the error message.
    \note User can make a copy of this function for localization.
        Using switch-case is safer for future modification of error codes.
*/
inline const MERAK_JSON_ERROR_CHARTYPE* GetValidateError_En(ValidateErrorCode validateErrorCode) {
    switch (validateErrorCode) {
        case kValidateErrors:                           return MERAK_JSON_ERROR_STRING("One or more validation errors have occurred");
        case kValidateErrorNone:                        return MERAK_JSON_ERROR_STRING("No error.");

        case kValidateErrorMultipleOf:                  return MERAK_JSON_ERROR_STRING("Number '%actual' is not a multiple of the 'multipleOf' value '%expected'.");
        case kValidateErrorMaximum:                     return MERAK_JSON_ERROR_STRING("Number '%actual' is greater than the 'maximum' value '%expected'.");
        case kValidateErrorExclusiveMaximum:            return MERAK_JSON_ERROR_STRING("Number '%actual' is greater than or equal to the 'exclusiveMaximum' value '%expected'.");
        case kValidateErrorMinimum:                     return MERAK_JSON_ERROR_STRING("Number '%actual' is less than the 'minimum' value '%expected'.");
        case kValidateErrorExclusiveMinimum:            return MERAK_JSON_ERROR_STRING("Number '%actual' is less than or equal to the 'exclusiveMinimum' value '%expected'.");

        case kValidateErrorMaxLength:                   return MERAK_JSON_ERROR_STRING("String '%actual' is longer than the 'maxLength' value '%expected'.");
        case kValidateErrorMinLength:                   return MERAK_JSON_ERROR_STRING("String '%actual' is shorter than the 'minLength' value '%expected'.");
        case kValidateErrorPattern:                     return MERAK_JSON_ERROR_STRING("String '%actual' does not match the 'pattern' regular expression.");

        case kValidateErrorMaxItems:                    return MERAK_JSON_ERROR_STRING("Array of length '%actual' is longer than the 'maxItems' value '%expected'.");
        case kValidateErrorMinItems:                    return MERAK_JSON_ERROR_STRING("Array of length '%actual' is shorter than the 'minItems' value '%expected'.");
        case kValidateErrorUniqueItems:                 return MERAK_JSON_ERROR_STRING("Array has duplicate items at indices '%duplicates' but 'uniqueItems' is true.");
        case kValidateErrorAdditionalItems:             return MERAK_JSON_ERROR_STRING("Array has an additional item at index '%disallowed' that is not allowed by the schema.");

        case kValidateErrorMaxProperties:               return MERAK_JSON_ERROR_STRING("Object has '%actual' members which is more than 'maxProperties' value '%expected'.");
        case kValidateErrorMinProperties:               return MERAK_JSON_ERROR_STRING("Object has '%actual' members which is less than 'minProperties' value '%expected'.");
        case kValidateErrorRequired:                    return MERAK_JSON_ERROR_STRING("Object is missing the following members required by the schema: '%missing'.");
        case kValidateErrorAdditionalProperties:        return MERAK_JSON_ERROR_STRING("Object has an additional member '%disallowed' that is not allowed by the schema.");
        case kValidateErrorPatternProperties:           return MERAK_JSON_ERROR_STRING("Object has 'patternProperties' that are not allowed by the schema.");
        case kValidateErrorDependencies:                return MERAK_JSON_ERROR_STRING("Object has missing property or schema dependencies, refer to following errors.");

        case kValidateErrorEnum:                        return MERAK_JSON_ERROR_STRING("Property has a value that is not one of its allowed enumerated values.");
        case kValidateErrorType:                        return MERAK_JSON_ERROR_STRING("Property has a type '%actual' that is not in the following list: '%expected'.");

        case kValidateErrorOneOf:                       return MERAK_JSON_ERROR_STRING("Property did not match any of the sub-schemas specified by 'oneOf', refer to following errors.");
        case kValidateErrorOneOfMatch:                  return MERAK_JSON_ERROR_STRING("Property matched more than one of the sub-schemas specified by 'oneOf', indices '%matches'.");
        case kValidateErrorAllOf:                       return MERAK_JSON_ERROR_STRING("Property did not match all of the sub-schemas specified by 'allOf', refer to following errors.");
        case kValidateErrorAnyOf:                       return MERAK_JSON_ERROR_STRING("Property did not match any of the sub-schemas specified by 'anyOf', refer to following errors.");
        case kValidateErrorNot:                         return MERAK_JSON_ERROR_STRING("Property matched the sub-schema specified by 'not'.");

        case kValidateErrorReadOnly:                    return MERAK_JSON_ERROR_STRING("Property is read-only but has been provided when validation is for writing.");
        case kValidateErrorWriteOnly:                   return MERAK_JSON_ERROR_STRING("Property is write-only but has been provided when validation is for reading.");

        default:                                        return MERAK_JSON_ERROR_STRING("Unknown error.");
    }
}

//! Maps error code of schema document compilation into error message.
/*!
    \ingroup MERAK_JSON_ERRORS
    \param schemaErrorCode Error code obtained from compiling the schema document.
    \return the error message.
    \note User can make a copy of this function for localization.
        Using switch-case is safer for future modification of error codes.
*/
  inline const MERAK_JSON_ERROR_CHARTYPE* GetSchemaError_En(SchemaErrorCode schemaErrorCode) {
      switch (schemaErrorCode) {
          case kSchemaErrorNone:                        return MERAK_JSON_ERROR_STRING("No error.");

          case kSchemaErrorStartUnknown:                return MERAK_JSON_ERROR_STRING("Pointer '%value' to start of schema does not resolve to a location in the document.");
          case kSchemaErrorRefPlainName:                return MERAK_JSON_ERROR_STRING("$ref fragment '%value' must be a JSON pointer.");
          case kSchemaErrorRefInvalid:                  return MERAK_JSON_ERROR_STRING("$ref must not be an empty string.");
          case kSchemaErrorRefPointerInvalid:           return MERAK_JSON_ERROR_STRING("$ref fragment '%value' is not a valid JSON pointer at offset '%offset'.");
          case kSchemaErrorRefUnknown:                  return MERAK_JSON_ERROR_STRING("$ref '%value' does not resolve to a location in the target document.");
          case kSchemaErrorRefCyclical:                 return MERAK_JSON_ERROR_STRING("$ref '%value' is cyclical.");
          case kSchemaErrorRefNoRemoteProvider:         return MERAK_JSON_ERROR_STRING("$ref is remote but there is no remote provider.");
          case kSchemaErrorRefNoRemoteSchema:           return MERAK_JSON_ERROR_STRING("$ref '%value' is remote but the remote provider did not return a schema.");
          case kSchemaErrorRegexInvalid:                return MERAK_JSON_ERROR_STRING("Invalid regular expression '%value' in 'pattern' or 'patternProperties'.");
          case kSchemaErrorSpecUnknown:                 return MERAK_JSON_ERROR_STRING("JSON schema draft or OpenAPI version is not recognized.");
          case kSchemaErrorSpecUnsupported:             return MERAK_JSON_ERROR_STRING("JSON schema draft or OpenAPI version is not supported.");
          case kSchemaErrorSpecIllegal:                 return MERAK_JSON_ERROR_STRING("Both JSON schema draft and OpenAPI version found in document.");
          case kSchemaErrorReadOnlyAndWriteOnly:        return MERAK_JSON_ERROR_STRING("Property must not be both 'readOnly' and 'writeOnly'.");

          default:                                      return MERAK_JSON_ERROR_STRING("Unknown error.");
    }
  }

//! Maps error code of pointer parse into error message.
/*!
    \ingroup MERAK_JSON_ERRORS
    \param pointerParseErrorCode Error code obtained from pointer parse.
    \return the error message.
    \note User can make a copy of this function for localization.
        Using switch-case is safer for future modification of error codes.
*/
inline const MERAK_JSON_ERROR_CHARTYPE* GetPointerParseError_En(PointerParseErrorCode pointerParseErrorCode) {
    switch (pointerParseErrorCode) {
        case kPointerParseErrorNone:                       return MERAK_JSON_ERROR_STRING("No error.");

        case kPointerParseErrorTokenMustBeginWithSolidus:  return MERAK_JSON_ERROR_STRING("A token must begin with a '/'.");
        case kPointerParseErrorInvalidEscape:              return MERAK_JSON_ERROR_STRING("Invalid escape.");
        case kPointerParseErrorInvalidPercentEncoding:     return MERAK_JSON_ERROR_STRING("Invalid percent encoding in URI fragment.");
        case kPointerParseErrorCharacterMustPercentEncode: return MERAK_JSON_ERROR_STRING("A character must be percent encoded in a URI fragment.");

        default:                                           return MERAK_JSON_ERROR_STRING("Unknown error.");
    }
}

}  // namespace merak::json

#ifdef __clang__
MERAK_JSON_DIAG_POP
#endif

#endif // MERAK_JSON_ERROR_EN_H_
