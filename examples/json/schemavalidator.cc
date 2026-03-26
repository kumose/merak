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
// Schema Validator example

// The example validates JSON text from stdin with a JSON schema specified in the argument.


#include <merak/json.h>
#include <string>
#include <iostream>
#include <sstream>

using namespace merak::json;

typedef GenericValue<UTF8<>, CrtAllocator > ValueType;

// Forward ref
static void CreateErrorMessages(const ValueType& errors, size_t depth, const char* context);

// Convert GenericValue to std::string
static std::string get_string(const ValueType& val) {
  std::ostringstream s;
  if (val.is_string())
    s << val.get_string();
  else if (val.is_double())
    s << val.get_double();
  else if (val.is_uint32())
   s << val.get_uint32();
  else if (val.is_int32())
    s << val.get_int32();
  else if (val.is_uint64())
    s << val.get_uint64();
  else if (val.is_int64())
    s <<  val.get_int64();
  else if (val.is_bool() && val.get_bool())
    s << "true";
  else if (val.is_bool())
    s << "false";
  else if (val.is_float())
    s << val.get_float();
  return s.str();
}

// Create the error message for a named error
// The error object can either be empty or contain at least member properties:
// {"errorCode": <code>, "instanceRef": "<pointer>", "schemaRef": "<pointer>" }
// Additional properties may be present for use as inserts.
// An "errors" property may be present if there are child errors.
static void HandleError(const char* errorName, const ValueType& error, size_t depth, const char* context) {
  if (!error.object_empty()) {
    // Get error code and look up error message text (English)
    int code = error["errorCode"].get_int32();
    std::string message(GetValidateError_En(static_cast<ValidateErrorCode>(code)));
    // For each member property in the error, see if its name exists as an insert in the error message and if so replace with the stringified property value
    // So for example - "Number '%actual' is not a multiple of the 'multipleOf' value '%expected'." - we would expect "actual" and "expected" members.
    for (ValueType::ConstMemberIterator insertsItr = error.member_begin();
      insertsItr != error.member_end(); ++insertsItr) {
      std::string insertName("%");
      insertName += insertsItr->name.get_string(); // eg "%actual"
      size_t insertPos = message.find(insertName);
      if (insertPos != std::string::npos) {
        std::string insertString("");
        const ValueType &insert = insertsItr->value;
        if (insert.is_array()) {
          // Member is an array so create comma-separated list of items for the insert string
          for (ValueType::ConstValueIterator itemsItr = insert.begin(); itemsItr != insert.end(); ++itemsItr) {
            if (itemsItr != insert.begin()) insertString += ",";
            insertString += get_string(*itemsItr);
          }
        } else {
          insertString += get_string(insert);
        }
        message.replace(insertPos, insertName.length(), insertString);
      }
    }
    // Output error message, references, context
    std::string indent(depth * 2, ' ');
    std::cout << indent << "Error Name: " << errorName << std::endl;
    std::cout << indent << "Message: " << message.c_str() << std::endl;
    std::cout << indent << "Instance: " << error["instanceRef"].get_string() << std::endl;
    std::cout << indent << "Schema: " << error["schemaRef"].get_string() << std::endl;
    if (depth > 0) std::cout << indent << "Context: " << context << std::endl;
    std::cout << std::endl;

    // If child errors exist, apply the process recursively to each error structure.
    // This occurs for "oneOf", "allOf", "anyOf" and "dependencies" errors, so pass the error name as context.
    if (error.has_member("errors")) {
      depth++;
      const ValueType &childErrors = error["errors"];
      if (childErrors.is_array()) {
        // Array - each item is an error structure - example
        // "anyOf": {"errorCode": ..., "errors":[{"pattern": {"errorCode\": ...\"}}, {"pattern": {"errorCode\": ...}}]
        for (ValueType::ConstValueIterator errorsItr = childErrors.begin();
             errorsItr != childErrors.end(); ++errorsItr) {
          CreateErrorMessages(*errorsItr, depth, errorName);
        }
      } else if (childErrors.is_object()) {
        // Object - each member is an error structure - example
        // "dependencies": {"errorCode": ..., "errors": {"address": {"required": {"errorCode": ...}}, "name": {"required": {"errorCode": ...}}}
        for (ValueType::ConstMemberIterator propsItr = childErrors.member_begin();
             propsItr != childErrors.member_end(); ++propsItr) {
          CreateErrorMessages(propsItr->value, depth, errorName);
        }
      }
    }
  }
}

// Create error message for all errors in an error structure
// Context is used to indicate whether the error structure has a parent 'dependencies', 'allOf', 'anyOf' or 'oneOf' error
static void CreateErrorMessages(const ValueType& errors, size_t depth = 0, const char* context = 0) {
    // Each member property contains one or more errors of a given type
    for (ValueType::ConstMemberIterator errorTypeItr = errors.member_begin(); errorTypeItr != errors.member_end(); ++errorTypeItr) {
        const char* errorName = errorTypeItr->name.get_string();
        const ValueType& errorContent = errorTypeItr->value;
        if (errorContent.is_array()) {
            // Member is an array where each item is an error - eg "type": [{"errorCode": ...}, {"errorCode": ...}]
            for (ValueType::ConstValueIterator contentItr = errorContent.begin(); contentItr != errorContent.end(); ++contentItr) {
                HandleError(errorName, *contentItr, depth, context);
            }
        } else if (errorContent.is_object()) {
            // Member is an object which is a single error - eg "type": {"errorCode": ... }
            HandleError(errorName, errorContent, depth, context);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: schemavalidator schema.json < input.json\n");
        return EXIT_FAILURE;
    }

    // Read a JSON schema from file into Document
    Document d;
    char buffer[4096];

    {
        FILE *fp = fopen(argv[1], "r");
        if (!fp) {
            printf("Schema file '%s' not found\n", argv[1]);
            return -1;
        }
        FileReadStream fs(fp, buffer, sizeof(buffer));
        d.parse_stream(fs);
        if (d.has_parse_error()) {
            fprintf(stderr, "Schema file '%s' is not a valid JSON\n", argv[1]);
            fprintf(stderr, "Error(offset %u): %s\n",
                static_cast<unsigned>(d.get_error_offset()),
                get_parse_error_en(d.get_parse_error()));
            fclose(fp);
            return EXIT_FAILURE;
        }
        fclose(fp);
    }
    
    // Then convert the Document into SchemaDocument
    SchemaDocument sd(d);

    // Use reader to parse the JSON in stdin, and forward SAX events to validator
    SchemaValidator validator(sd);
    Reader reader;
    FileReadStream is(stdin, buffer, sizeof(buffer));
    if (!reader.parse(is, validator) && reader.GetParseErrorCode() != kParseErrorTermination) {
        // Schema validator error would cause kParseErrorTermination, which will handle it in next step.
        fprintf(stderr, "Input is not a valid JSON\n");
        fprintf(stderr, "Error(offset %u): %s\n",
            static_cast<unsigned>(reader.get_error_offset()),
            get_parse_error_en(reader.GetParseErrorCode()));
    }

    // Check the validation result
    if (validator.IsValid()) {
        printf("Input JSON is valid.\n");
        return EXIT_SUCCESS;
    }
    else {
        printf("Input JSON is invalid.\n");
        StringBuffer sb;
        validator.GetInvalidSchemaPointer().StringifyUriFragment(sb);
        fprintf(stderr, "Invalid schema: %s\n", sb.get_string());
        fprintf(stderr, "Invalid keyword: %s\n", validator.GetInvalidSchemaKeyword());
        fprintf(stderr, "Invalid code: %d\n", validator.GetInvalidSchemaCode());
        fprintf(stderr, "Invalid message: %s\n", GetValidateError_En(validator.GetInvalidSchemaCode()));
        sb.clear();
        validator.GetInvalidDocumentPointer().StringifyUriFragment(sb);
        fprintf(stderr, "Invalid document: %s\n", sb.get_string());
        // Detailed violation report is available as a JSON value
        sb.clear();
        PrettyWriter<StringBuffer> w(sb);
        validator.GetError().accept(w);
        fprintf(stderr, "Error report:\n%s\n", sb.get_string());
        CreateErrorMessages(validator.GetError());
        return EXIT_FAILURE;
    }
}
