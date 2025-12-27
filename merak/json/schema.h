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

#ifndef MERAK_JSON_SCHEMA_H_
#define MERAK_JSON_SCHEMA_H_

#include <merak/json/document.h>
#include <merak/json/pointer.h>
#include <merak/json/stringbuffer.h>
#include <merak/json/error/en.h>
#include <merak/json/uri.h>
#include <cmath> // abs, floor

#if !defined(MERAK_JSON_SCHEMA_USE_INTERNALREGEX)
#define MERAK_JSON_SCHEMA_USE_INTERNALREGEX 1
#endif

#if !defined(MERAK_JSON_SCHEMA_USE_STDREGEX) || !(__cplusplus >= 201103L || (defined(_MSC_VER) && _MSC_VER >= 1800))
#define MERAK_JSON_SCHEMA_USE_STDREGEX 0
#endif

#if MERAK_JSON_SCHEMA_USE_INTERNALREGEX

#include <merak/json/internal/regex.h>

#elif MERAK_JSON_SCHEMA_USE_STDREGEX
#include <regex>
#endif

#if MERAK_JSON_SCHEMA_USE_INTERNALREGEX || MERAK_JSON_SCHEMA_USE_STDREGEX
#define MERAK_JSON_SCHEMA_HAS_REGEX 1
#else
#define MERAK_JSON_SCHEMA_HAS_REGEX 0
#endif

#ifndef MERAK_JSON_SCHEMA_VERBOSE
#define MERAK_JSON_SCHEMA_VERBOSE 0
#endif

MERAK_JSON_DIAG_PUSH

#if defined(__GNUC__)
MERAK_JSON_DIAG_OFF(effc++)
#endif

#ifdef __clang__
MERAK_JSON_DIAG_OFF(weak-vtables)
MERAK_JSON_DIAG_OFF(exit-time-destructors)
MERAK_JSON_DIAG_OFF(c++98-compat-pedantic)
MERAK_JSON_DIAG_OFF(variadic-macros)
#elif defined(_MSC_VER)
MERAK_JSON_DIAG_OFF(4512) // assignment operator could not be generated
#endif

namespace merak::json {

    ///////////////////////////////////////////////////////////////////////////////
    // Verbose Utilities

#if MERAK_JSON_SCHEMA_VERBOSE

    namespace internal {

    inline void PrintInvalidKeywordData(const char* keyword) {
        printf("    Fail keyword: '%s'\n", keyword);
    }

    inline void PrintInvalidKeywordData(const wchar_t* keyword) {
        wprintf(L"    Fail keyword: '%ls'\n", keyword);
    }

    inline void PrintInvalidDocumentData(const char* document) {
        printf("    Fail document: '%s'\n", document);
    }

    inline void PrintInvalidDocumentData(const wchar_t* document) {
        wprintf(L"    Fail document: '%ls'\n", document);
    }

    inline void PrintValidatorPointersData(const char* s, const char* d, unsigned depth) {
        printf("    Sch: %*s'%s'\n    Doc: %*s'%s'\n", depth * 4, " ", s, depth * 4, " ", d);
    }

    inline void PrintValidatorPointersData(const wchar_t* s, const wchar_t* d, unsigned depth) {
        wprintf(L"    Sch: %*ls'%ls'\n    Doc: %*ls'%ls'\n", depth * 4, L" ", s, depth * 4, L" ", d);
    }

    inline void PrintSchemaIdsData(const char* base, const char* local, const char* resolved) {
        printf("    Resolving id: Base: '%s', Local: '%s', Resolved: '%s'\n", base, local, resolved);
    }

    inline void PrintSchemaIdsData(const wchar_t* base, const wchar_t* local, const wchar_t* resolved) {
        wprintf(L"    Resolving id: Base: '%ls', Local: '%ls', Resolved: '%ls'\n", base, local, resolved);
    }

    inline void PrintMethodData(const char* method) {
        printf("%s\n", method);
    }

    inline void PrintMethodData(const char* method, bool b) {
        printf("%s, Data: '%s'\n", method, b ? "true" : "false");
    }

    inline void PrintMethodData(const char* method, int64_t i) {
        printf("%s, Data: '%" PRId64 "'\n", method, i);
    }

    inline void PrintMethodData(const char* method, uint64_t u) {
        printf("%s, Data: '%" PRIu64 "'\n", method, u);
    }

    inline void PrintMethodData(const char* method, double d) {
        printf("%s, Data: '%lf'\n", method, d);
    }

    inline void PrintMethodData(const char* method, const char* s) {
        printf("%s, Data: '%s'\n", method, s);
    }

    inline void PrintMethodData(const char* method, const wchar_t* s) {
        wprintf(L"%hs, Data: '%ls'\n", method, s);
    }

    inline void PrintMethodData(const char* method, const char* s1, const char* s2) {
        printf("%s, Data: '%s', '%s'\n", method, s1, s2);
    }

    inline void PrintMethodData(const char* method, const wchar_t* s1, const wchar_t* s2) {
        wprintf(L"%hs, Data: '%ls', '%ls'\n", method, s1, s2);
    }

    } // namespace internal

#endif // MERAK_JSON_SCHEMA_VERBOSE

#ifndef MERAK_JSON_SCHEMA_PRINT
#if MERAK_JSON_SCHEMA_VERBOSE
#define MERAK_JSON_SCHEMA_PRINT(name, ...) internal::Print##name##Data(__VA_ARGS__)
#else
#define MERAK_JSON_SCHEMA_PRINT(name, ...)
#endif
#endif

    ///////////////////////////////////////////////////////////////////////////////
    // MERAK_JSON_INVALID_KEYWORD_RETURN

#define MERAK_JSON_INVALID_KEYWORD_RETURN(code)\
    MERAK_JSON_MULTILINEMACRO_BEGIN\
        context.invalidCode = code;\
        context.invalidKeyword = SchemaType::GetValidateErrorKeyword(code).get_string();\
        MERAK_JSON_SCHEMA_PRINT(InvalidKeyword, context.invalidKeyword);\
        return false;\
    MERAK_JSON_MULTILINEMACRO_END

    ///////////////////////////////////////////////////////////////////////////////
    // ValidateFlag

    /*! \def MERAK_JSON_VALIDATE_DEFAULT_FLAGS
        \ingroup MERAK_JSON_CONFIG
        \brief User-defined kValidateDefaultFlags definition.

        User can define this as any \c ValidateFlag combinations.
    */
#ifndef MERAK_JSON_VALIDATE_DEFAULT_FLAGS
#define MERAK_JSON_VALIDATE_DEFAULT_FLAGS kValidateNoFlags
#endif

    //! Combination of validate flags
    enum ValidateFlag {
        kValidateNoFlags = 0,                                       //!< No flags are set.
        kValidateContinueOnErrorFlag = 1,                           //!< Don't stop after first validation error.
        kValidateReadFlag = 2,                                      //!< Validation is for a read semantic.
        kValidateWriteFlag = 4,                                     //!< Validation is for a write semantic.
        kValidateDefaultFlags = MERAK_JSON_VALIDATE_DEFAULT_FLAGS    //!< Default validate flags. Can be customized by defining MERAK_JSON_VALIDATE_DEFAULT_FLAGS
    };

    ///////////////////////////////////////////////////////////////////////////////
    // Specification
    enum SchemaDraft {
        kDraftUnknown = -1,
        kDraftNone = 0,
        kDraft03 = 3,
        kDraftMin = 4,                       //!< Current minimum supported draft
        kDraft04 = 4,
        kDraft05 = 5,
        kDraftMax = 5,                       //!< Current maximum supported draft
        kDraft06 = 6,
        kDraft07 = 7,
        kDraft2019_09 = 8,
        kDraft2020_12 = 9
    };

    enum OpenApiVersion {
        kVersionUnknown = -1,
        kVersionNone = 0,
        kVersionMin = 2,                      //!< Current minimum supported version
        kVersion20 = 2,
        kVersion30 = 3,
        kVersionMax = 3,                      //!< Current maximum supported version
        kVersion31 = 4,
    };

    struct Specification {
        Specification(SchemaDraft d) : draft(d), oapi(kVersionNone) {}

        Specification(OpenApiVersion o) : oapi(o) {
            if (oapi == kVersion20) draft = kDraft04;
            else if (oapi == kVersion30) draft = kDraft05;
            else if (oapi == kVersion31) draft = kDraft2020_12;
            else draft = kDraft04;
        }

        ~Specification() {}

        bool IsSupported() const {
            return ((draft >= kDraftMin && draft <= kDraftMax) &&
                    ((oapi == kVersionNone) || (oapi >= kVersionMin && oapi <= kVersionMax)));
        }

        SchemaDraft draft;
        OpenApiVersion oapi;
    };

    ///////////////////////////////////////////////////////////////////////////////
    // Forward declarations

    template<typename ValueType, typename Allocator>
    class GenericSchemaDocument;

    namespace internal {

        template<typename SchemaDocumentType>
        class Schema;

        ///////////////////////////////////////////////////////////////////////////////
        // ISchemaValidator

        class ISchemaValidator {
        public:
            virtual ~ISchemaValidator() {}

            virtual bool IsValid() const = 0;

            virtual void SetValidateFlags(unsigned flags) = 0;

            virtual unsigned GetValidateFlags() const = 0;
        };

        ///////////////////////////////////////////////////////////////////////////////
        // ISchemaStateFactory

        template<typename SchemaType>
        class ISchemaStateFactory {
        public:
            virtual ~ISchemaStateFactory() {}

            virtual ISchemaValidator *CreateSchemaValidator(const SchemaType &, const bool inheritContinueOnErrors) = 0;

            virtual void DestroySchemaValidator(ISchemaValidator *validator) = 0;

            virtual void *CreateHasher() = 0;

            virtual uint64_t GetHashCode(void *hasher) = 0;

            virtual void DestroryHasher(void *hasher) = 0;

            virtual void *MallocState(size_t size) = 0;

            virtual void FreeState(void *p) = 0;
        };

        ///////////////////////////////////////////////////////////////////////////////
        // IValidationErrorHandler

        template<typename SchemaType>
        class IValidationErrorHandler {
        public:
            typedef typename SchemaType::char_type char_type;
            typedef typename SchemaType::SValue SValue;

            virtual ~IValidationErrorHandler() {}

            virtual void NotMultipleOf(int64_t actual, const SValue &expected) = 0;

            virtual void NotMultipleOf(uint64_t actual, const SValue &expected) = 0;

            virtual void NotMultipleOf(double actual, const SValue &expected) = 0;

            virtual void AboveMaximum(int64_t actual, const SValue &expected, bool exclusive) = 0;

            virtual void AboveMaximum(uint64_t actual, const SValue &expected, bool exclusive) = 0;

            virtual void AboveMaximum(double actual, const SValue &expected, bool exclusive) = 0;

            virtual void BelowMinimum(int64_t actual, const SValue &expected, bool exclusive) = 0;

            virtual void BelowMinimum(uint64_t actual, const SValue &expected, bool exclusive) = 0;

            virtual void BelowMinimum(double actual, const SValue &expected, bool exclusive) = 0;

            virtual void TooLong(const char_type *str, SizeType length, SizeType expected) = 0;

            virtual void TooShort(const char_type *str, SizeType length, SizeType expected) = 0;

            virtual void DoesNotMatch(const char_type *str, SizeType length) = 0;

            virtual void DisallowedItem(SizeType index) = 0;

            virtual void TooFewItems(SizeType actualCount, SizeType expectedCount) = 0;

            virtual void TooManyItems(SizeType actualCount, SizeType expectedCount) = 0;

            virtual void DuplicateItems(SizeType index1, SizeType index2) = 0;

            virtual void TooManyProperties(SizeType actualCount, SizeType expectedCount) = 0;

            virtual void TooFewProperties(SizeType actualCount, SizeType expectedCount) = 0;

            virtual void StartMissingProperties() = 0;

            virtual void AddMissingProperty(const SValue &name) = 0;

            virtual bool EndMissingProperties() = 0;

            virtual void PropertyViolations(ISchemaValidator **subvalidators, SizeType count) = 0;

            virtual void DisallowedProperty(const char_type *name, SizeType length) = 0;

            virtual void StartDependencyErrors() = 0;

            virtual void StartMissingDependentProperties() = 0;

            virtual void AddMissingDependentProperty(const SValue &targetName) = 0;

            virtual void EndMissingDependentProperties(const SValue &sourceName) = 0;

            virtual void AddDependencySchemaError(const SValue &souceName, ISchemaValidator *subvalidator) = 0;

            virtual bool EndDependencyErrors() = 0;

            virtual void DisallowedValue(const ValidateErrorCode code) = 0;

            virtual void StartDisallowedType() = 0;

            virtual void AddExpectedType(const typename SchemaType::ValueType &expectedType) = 0;

            virtual void EndDisallowedType(const typename SchemaType::ValueType &actualType) = 0;

            virtual void NotAllOf(ISchemaValidator **subvalidators, SizeType count) = 0;

            virtual void NoneOf(ISchemaValidator **subvalidators, SizeType count) = 0;

            virtual void NotOneOf(ISchemaValidator **subvalidators, SizeType count) = 0;

            virtual void MultipleOneOf(SizeType index1, SizeType index2) = 0;

            virtual void Disallowed() = 0;

            virtual void DisallowedWhenWriting() = 0;

            virtual void DisallowedWhenReading() = 0;
        };


        ///////////////////////////////////////////////////////////////////////////////
        // Hasher

        // For comparison of compound value
        template<typename Encoding, typename Allocator>
        class Hasher {
        public:
            typedef typename Encoding::char_type char_type;

            Hasher(Allocator *allocator = 0, size_t stackCapacity = kDefaultSize) : stack_(allocator, stackCapacity) {}

            bool emplace_null() { return WriteType(kNullType); }

            bool emplace_bool(bool b) { return WriteType(b ? kTrueType : kFalseType); }

            bool emplace_int32(int i) {
                Number n;
                n.u.i = i;
                n.d = static_cast<double>(i);
                return WriteNumber(n);
            }

            bool emplace_uint32(unsigned u) {
                Number n;
                n.u.u = u;
                n.d = static_cast<double>(u);
                return WriteNumber(n);
            }

            bool emplace_int64(int64_t i) {
                Number n;
                n.u.i = i;
                n.d = static_cast<double>(i);
                return WriteNumber(n);
            }

            bool emplace_uint64(uint64_t u) {
                Number n;
                n.u.u = u;
                n.d = static_cast<double>(u);
                return WriteNumber(n);
            }

            bool emplace_double(double d) {
                Number n;
                if (d < 0) n.u.i = static_cast<int64_t>(d);
                else n.u.u = static_cast<uint64_t>(d);
                n.d = d;
                return WriteNumber(n);
            }

            bool raw_number(const char_type *str, SizeType len, bool) {
                WriteBuffer(kNumberType, str, len * sizeof(char_type));
                return true;
            }

            bool emplace_string(const char_type *str, SizeType len, bool) {
                WriteBuffer(kStringType, str, len * sizeof(char_type));
                return true;
            }

            bool start_object() { return true; }

            bool emplace_key(const char_type *str, SizeType len, bool copy) { return emplace_string(str, len, copy); }

            bool end_object(SizeType memberCount) {
                uint64_t h = Hash(0, kObjectType);
                uint64_t *kv = stack_.template Pop<uint64_t>(memberCount * 2);
                for (SizeType i = 0; i < memberCount; i++)
                    // Issue #2205
                    // Hasing the key to avoid key=value cases with bug-prone zero-value hash
                    h ^= Hash(Hash(0, kv[i * 2]), kv[i * 2 + 1]);  // Use xor to achieve member order insensitive
                *stack_.template Push<uint64_t>() = h;
                return true;
            }

            bool start_array() { return true; }

            bool end_array(SizeType elementCount) {
                uint64_t h = Hash(0, kArrayType);
                uint64_t *e = stack_.template Pop<uint64_t>(elementCount);
                for (SizeType i = 0; i < elementCount; i++)
                    h = Hash(h, e[i]); // Use hash to achieve element order sensitive
                *stack_.template Push<uint64_t>() = h;
                return true;
            }

            bool IsValid() const { return stack_.GetSize() == sizeof(uint64_t); }

            uint64_t GetHashCode() const {
                MERAK_JSON_ASSERT(IsValid());
                return *stack_.template Top<uint64_t>();
            }

        private:
            static const size_t kDefaultSize = 256;
            struct Number {
                union U {
                    uint64_t u;
                    int64_t i;
                } u;
                double d;
            };

            bool WriteType(Type type) { return WriteBuffer(type, 0, 0); }

            bool WriteNumber(const Number &n) { return WriteBuffer(kNumberType, &n, sizeof(n)); }

            bool WriteBuffer(Type type, const void *data, size_t len) {
                // FNV-1a from http://isthe.com/chongo/tech/comp/fnv/
                uint64_t h = Hash(MERAK_JSON_UINT64_C2(0xcbf29ce4, 0x84222325), type);
                const unsigned char *d = static_cast<const unsigned char *>(data);
                for (size_t i = 0; i < len; i++)
                    h = Hash(h, d[i]);
                *stack_.template Push<uint64_t>() = h;
                return true;
            }

            static uint64_t Hash(uint64_t h, uint64_t d) {
                static const uint64_t kPrime = MERAK_JSON_UINT64_C2(0x00000100, 0x000001b3);
                h ^= d;
                h *= kPrime;
                return h;
            }

            Stack<Allocator> stack_;
        };

        ///////////////////////////////////////////////////////////////////////////////
        // SchemaValidationContext

        template<typename SchemaDocumentType>
        struct SchemaValidationContext {
            typedef Schema<SchemaDocumentType> SchemaType;
            typedef ISchemaStateFactory<SchemaType> SchemaValidatorFactoryType;
            typedef IValidationErrorHandler<SchemaType> ErrorHandlerType;
            typedef typename SchemaType::ValueType ValueType;
            typedef typename ValueType::char_type char_type;

            enum PatternValidatorType {
                kPatternValidatorOnly,
                kPatternValidatorWithProperty,
                kPatternValidatorWithAdditionalProperty
            };

            SchemaValidationContext(SchemaValidatorFactoryType &f, ErrorHandlerType &eh, const SchemaType *s,
                                    unsigned fl = 0) :
                    factory(f),
                    error_handler(eh),
                    schema(s),
                    flags(fl),
                    valueSchema(),
                    invalidKeyword(),
                    invalidCode(),
                    hasher(),
                    arrayElementHashCodes(),
                    validators(),
                    validatorCount(),
                    patternPropertiesValidators(),
                    patternPropertiesValidatorCount(),
                    patternPropertiesSchemas(),
                    patternPropertiesSchemaCount(),
                    valuePatternValidatorType(kPatternValidatorOnly),
                    propertyExist(),
                    inArray(false),
                    valueUniqueness(false),
                    arrayUniqueness(false) {
            }

            ~SchemaValidationContext() {
                if (hasher)
                    factory.DestroryHasher(hasher);
                if (validators) {
                    for (SizeType i = 0; i < validatorCount; i++) {
                        if (validators[i]) {
                            factory.DestroySchemaValidator(validators[i]);
                        }
                    }
                    factory.FreeState(validators);
                }
                if (patternPropertiesValidators) {
                    for (SizeType i = 0; i < patternPropertiesValidatorCount; i++) {
                        if (patternPropertiesValidators[i]) {
                            factory.DestroySchemaValidator(patternPropertiesValidators[i]);
                        }
                    }
                    factory.FreeState(patternPropertiesValidators);
                }
                if (patternPropertiesSchemas)
                    factory.FreeState(patternPropertiesSchemas);
                if (propertyExist)
                    factory.FreeState(propertyExist);
            }

            SchemaValidatorFactoryType &factory;
            ErrorHandlerType &error_handler;
            const SchemaType *schema;
            unsigned flags;
            const SchemaType *valueSchema;
            const char_type *invalidKeyword;
            ValidateErrorCode invalidCode;
            void *hasher; // Only validator access
            void *arrayElementHashCodes; // Only validator access this
            ISchemaValidator **validators;
            SizeType validatorCount;
            ISchemaValidator **patternPropertiesValidators;
            SizeType patternPropertiesValidatorCount;
            const SchemaType **patternPropertiesSchemas;
            SizeType patternPropertiesSchemaCount;
            PatternValidatorType valuePatternValidatorType;
            PatternValidatorType objectPatternValidatorType;
            SizeType arrayElementIndex;
            bool *propertyExist;
            bool inArray;
            bool valueUniqueness;
            bool arrayUniqueness;
        };

        ///////////////////////////////////////////////////////////////////////////////
        // Schema

        template<typename SchemaDocumentType>
        class Schema {
        public:
            typedef typename SchemaDocumentType::ValueType ValueType;
            typedef typename SchemaDocumentType::AllocatorType AllocatorType;
            typedef typename SchemaDocumentType::PointerType PointerType;
            typedef typename ValueType::EncodingType EncodingType;
            typedef typename EncodingType::char_type char_type;
            typedef SchemaValidationContext<SchemaDocumentType> Context;
            typedef Schema<SchemaDocumentType> SchemaType;
            typedef GenericValue<EncodingType, AllocatorType> SValue;
            typedef IValidationErrorHandler<Schema> ErrorHandler;
            typedef GenericUri<ValueType, AllocatorType> UriType;

            friend class GenericSchemaDocument<ValueType, AllocatorType>;

            Schema(SchemaDocumentType *schemaDocument, const PointerType &p, const ValueType &value,
                   const ValueType &document, AllocatorType *allocator, const UriType &id = UriType()) :
                    allocator_(allocator),
                    uri_(schemaDocument->GetURI(), *allocator),
                    id_(id, allocator),
                    spec_(schemaDocument->GetSpecification()),
                    pointer_(p, allocator),
                    typeless_(schemaDocument->GetTypeless()),
                    enum_(),
                    enumCount_(),
                    not_(),
                    type_((1 << kTotalSchemaType) - 1), // typeless
                    validatorCount_(),
                    notValidatorIndex_(),
                    properties_(),
                    additionalPropertiesSchema_(),
                    patternProperties_(),
                    patternPropertyCount_(),
                    propertyCount_(),
                    minProperties_(),
                    maxProperties_(SizeType(~0)),
                    additionalProperties_(true),
                    hasDependencies_(),
                    hasRequired_(),
                    hasSchemaDependencies_(),
                    additionalItemsSchema_(),
                    itemsList_(),
                    itemsTuple_(),
                    itemsTupleCount_(),
                    minItems_(),
                    maxItems_(SizeType(~0)),
                    additionalItems_(true),
                    uniqueItems_(false),
                    pattern_(),
                    minLength_(0),
                    maxLength_(~SizeType(0)),
                    exclusiveMinimum_(false),
                    exclusiveMaximum_(false),
                    defaultValueLength_(0),
                    readOnly_(false),
                    writeOnly_(false),
                    nullable_(false) {
                GenericStringBuffer<EncodingType> sb;
                p.StringifyUriFragment(sb);
                MERAK_JSON_SCHEMA_PRINT(Method, "Schema::Schema", sb.get_string(), id.get_string());

                typedef typename ValueType::ConstValueIterator ConstValueIterator;
                typedef typename ValueType::ConstMemberIterator ConstMemberIterator;

                // PR #1393
                // Early add this Schema and its $ref(s) in schemaDocument's map to avoid infinite
                // recursion (with recursive schemas), since schemaDocument->getSchema() is always
                // checked before creating a new one. Don't cache typeless_, though.
                if (this != typeless_) {
                    typedef typename SchemaDocumentType::SchemaEntry SchemaEntry;
                    SchemaEntry *entry = schemaDocument->schemaMap_.template Push<SchemaEntry>();
                    new(entry) SchemaEntry(pointer_, this, true, allocator_);
                    schemaDocument->AddSchemaRefs(this);
                }

                if (!value.is_object())
                    return;

                // If we have an id property, resolve it with the in-scope id
                // Not supported for open api 2.0 or 3.0
                if (spec_.oapi != kVersion20 && spec_.oapi != kVersion30)
                    if (const ValueType *v = GetMember(value, GetIdString())) {
                        if (v->is_string()) {
                            UriType local(*v, allocator);
                            id_ = local.Resolve(id_, allocator);
                            MERAK_JSON_SCHEMA_PRINT(SchemaIds, id.get_string(), v->get_string(), id_.get_string());
                        }
                    }

                if (const ValueType *v = GetMember(value, GetTypeString())) {
                    type_ = 0;
                    if (v->is_string())
                        AddType(*v);
                    else if (v->is_array())
                        for (ConstValueIterator itr = v->begin(); itr != v->end(); ++itr)
                            AddType(*itr);
                }

                if (const ValueType *v = GetMember(value, GetEnumString())) {
                    if (v->is_array() && v->size() > 0) {
                        enum_ = static_cast<uint64_t *>(allocator_->Malloc(sizeof(uint64_t) * v->size()));
                        for (ConstValueIterator itr = v->begin(); itr != v->end(); ++itr) {
                            typedef Hasher<EncodingType, MemoryPoolAllocator<AllocatorType> > EnumHasherType;
                            char buffer[256u + 24];
                            MemoryPoolAllocator<AllocatorType> hasherAllocator(buffer, sizeof(buffer));
                            EnumHasherType h(&hasherAllocator, 256);
                            itr->accept(h);
                            enum_[enumCount_++] = h.GetHashCode();
                        }
                    }
                }

                if (schemaDocument)
                    AssignIfExist(allOf_, *schemaDocument, p, value, GetAllOfString(), document);

                // AnyOf, OneOf, Not not supported for open api 2.0
                if (schemaDocument && spec_.oapi != kVersion20) {
                    AssignIfExist(anyOf_, *schemaDocument, p, value, GetAnyOfString(), document);
                    AssignIfExist(oneOf_, *schemaDocument, p, value, GetOneOfString(), document);

                    if (const ValueType *v = GetMember(value, GetNotString())) {
                        schemaDocument->CreateSchema(&not_, p.Append(GetNotString(), allocator_), *v, document, id_);
                        notValidatorIndex_ = validatorCount_;
                        validatorCount_++;
                    }
                }

                // Object

                const ValueType *properties = GetMember(value, GetPropertiesString());
                const ValueType *required = GetMember(value, GetRequiredString());
                const ValueType *dependencies = GetMember(value, GetDependenciesString());
                {
                    // Gather properties from properties/required/dependencies
                    SValue allProperties(kArrayType);

                    if (properties && properties->is_object())
                        for (ConstMemberIterator itr = properties->member_begin(); itr != properties->member_end(); ++itr)
                            AddUniqueElement(allProperties, itr->name);

                    if (required && required->is_array())
                        for (ConstValueIterator itr = required->begin(); itr != required->end(); ++itr)
                            if (itr->is_string())
                                AddUniqueElement(allProperties, *itr);

                    // Dependencies not supported for open api 2.0 and 3.0
                    if (spec_.oapi != kVersion20 && spec_.oapi != kVersion30)
                        if (dependencies && dependencies->is_object())
                            for (ConstMemberIterator itr = dependencies->member_begin();
                                 itr != dependencies->member_end(); ++itr) {
                                AddUniqueElement(allProperties, itr->name);
                                if (itr->value.is_array())
                                    for (ConstValueIterator i = itr->value.begin(); i != itr->value.end(); ++i)
                                        if (i->is_string())
                                            AddUniqueElement(allProperties, *i);
                            }

                    if (allProperties.size() > 0) {
                        propertyCount_ = allProperties.size();
                        properties_ = static_cast<Property *>(allocator_->Malloc(sizeof(Property) * propertyCount_));
                        for (SizeType i = 0; i < propertyCount_; i++) {
                            new(&properties_[i]) Property();
                            properties_[i].name = allProperties[i];
                            properties_[i].schema = typeless_;
                        }
                    }
                }

                if (properties && properties->is_object()) {
                    PointerType q = p.Append(GetPropertiesString(), allocator_);
                    for (ConstMemberIterator itr = properties->member_begin(); itr != properties->member_end(); ++itr) {
                        SizeType index;
                        if (FindPropertyIndex(itr->name, &index))
                            schemaDocument->CreateSchema(&properties_[index].schema, q.Append(itr->name, allocator_),
                                                         itr->value, document, id_);
                    }
                }

                // PatternProperties not supported for open api 2.0 and 3.0
                if (spec_.oapi != kVersion20 && spec_.oapi != kVersion30)
                    if (const ValueType *v = GetMember(value, GetPatternPropertiesString())) {
                        PointerType q = p.Append(GetPatternPropertiesString(), allocator_);
                        patternProperties_ = static_cast<PatternProperty *>(allocator_->Malloc(
                                sizeof(PatternProperty) * v->member_count()));
                        patternPropertyCount_ = 0;

                        for (ConstMemberIterator itr = v->member_begin(); itr != v->member_end(); ++itr) {
                            new(&patternProperties_[patternPropertyCount_]) PatternProperty();
                            PointerType r = q.Append(itr->name, allocator_);
                            patternProperties_[patternPropertyCount_].pattern = CreatePattern(itr->name, schemaDocument,
                                                                                              r);
                            schemaDocument->CreateSchema(&patternProperties_[patternPropertyCount_].schema, r,
                                                         itr->value, document, id_);
                            patternPropertyCount_++;
                        }
                    }

                if (required && required->is_array())
                    for (ConstValueIterator itr = required->begin(); itr != required->end(); ++itr)
                        if (itr->is_string()) {
                            SizeType index;
                            if (FindPropertyIndex(*itr, &index)) {
                                properties_[index].required = true;
                                hasRequired_ = true;
                            }
                        }

                // Dependencies not supported for open api 2.0 and 3.0
                if (spec_.oapi != kVersion20 && spec_.oapi != kVersion30)
                    if (dependencies && dependencies->is_object()) {
                        PointerType q = p.Append(GetDependenciesString(), allocator_);
                        hasDependencies_ = true;
                        for (ConstMemberIterator itr = dependencies->member_begin();
                             itr != dependencies->member_end(); ++itr) {
                            SizeType sourceIndex;
                            if (FindPropertyIndex(itr->name, &sourceIndex)) {
                                if (itr->value.is_array()) {
                                    properties_[sourceIndex].dependencies = static_cast<bool *>(allocator_->Malloc(
                                            sizeof(bool) * propertyCount_));
                                    std::memset(properties_[sourceIndex].dependencies, 0,
                                                sizeof(bool) * propertyCount_);
                                    for (ConstValueIterator targetItr = itr->value.begin();
                                         targetItr != itr->value.end(); ++targetItr) {
                                        SizeType targetIndex;
                                        if (FindPropertyIndex(*targetItr, &targetIndex))
                                            properties_[sourceIndex].dependencies[targetIndex] = true;
                                    }
                                } else if (itr->value.is_object()) {
                                    hasSchemaDependencies_ = true;
                                    schemaDocument->CreateSchema(&properties_[sourceIndex].dependenciesSchema,
                                                                 q.Append(itr->name, allocator_), itr->value, document,
                                                                 id_);
                                    properties_[sourceIndex].dependenciesValidatorIndex = validatorCount_;
                                    validatorCount_++;
                                }
                            }
                        }
                    }

                if (const ValueType *v = GetMember(value, GetAdditionalPropertiesString())) {
                    if (v->is_bool())
                        additionalProperties_ = v->get_bool();
                    else if (v->is_object())
                        schemaDocument->CreateSchema(&additionalPropertiesSchema_,
                                                     p.Append(GetAdditionalPropertiesString(), allocator_), *v,
                                                     document, id_);
                }

                AssignIfExist(minProperties_, value, GetMinPropertiesString());
                AssignIfExist(maxProperties_, value, GetMaxPropertiesString());

                // Array
                if (const ValueType *v = GetMember(value, GetItemsString())) {
                    PointerType q = p.Append(GetItemsString(), allocator_);
                    if (v->is_object()) // List validation
                        schemaDocument->CreateSchema(&itemsList_, q, *v, document, id_);
                    else if (v->is_array()) { // Tuple validation
                        itemsTuple_ = static_cast<const Schema **>(allocator_->Malloc(
                                sizeof(const Schema *) * v->size()));
                        SizeType index = 0;
                        for (ConstValueIterator itr = v->begin(); itr != v->end(); ++itr, index++)
                            schemaDocument->CreateSchema(&itemsTuple_[itemsTupleCount_++], q.Append(index, allocator_),
                                                         *itr, document, id_);
                    }
                }

                AssignIfExist(minItems_, value, GetMinItemsString());
                AssignIfExist(maxItems_, value, GetMaxItemsString());

                // AdditionalItems not supported for openapi 2.0 and 3.0
                if (spec_.oapi != kVersion20 && spec_.oapi != kVersion30)
                    if (const ValueType *v = GetMember(value, GetAdditionalItemsString())) {
                        if (v->is_bool())
                            additionalItems_ = v->get_bool();
                        else if (v->is_object())
                            schemaDocument->CreateSchema(&additionalItemsSchema_,
                                                         p.Append(GetAdditionalItemsString(), allocator_), *v, document,
                                                         id_);
                    }

                AssignIfExist(uniqueItems_, value, GetUniqueItemsString());

                // String
                AssignIfExist(minLength_, value, GetMinLengthString());
                AssignIfExist(maxLength_, value, GetMaxLengthString());

                if (const ValueType *v = GetMember(value, GetPatternString()))
                    pattern_ = CreatePattern(*v, schemaDocument, p.Append(GetPatternString(), allocator_));

                // Number
                if (const ValueType *v = GetMember(value, GetMinimumString()))
                    if (v->is_number())
                        minimum_.copy_from(*v, *allocator_);

                if (const ValueType *v = GetMember(value, GetMaximumString()))
                    if (v->is_number())
                        maximum_.copy_from(*v, *allocator_);

                AssignIfExist(exclusiveMinimum_, value, GetExclusiveMinimumString());
                AssignIfExist(exclusiveMaximum_, value, GetExclusiveMaximumString());

                if (const ValueType *v = GetMember(value, GetMultipleOfString()))
                    if (v->is_number() && v->get_double() > 0.0)
                        multipleOf_.copy_from(*v, *allocator_);

                // Default
                if (const ValueType *v = GetMember(value, GetDefaultValueString()))
                    if (v->is_string())
                        defaultValueLength_ = v->get_string_length();

                // ReadOnly - open api only (until draft 7 supported)
                // WriteOnly - open api 3 only (until draft 7 supported)
                // Both can't be true
                if (spec_.oapi != kVersionNone)
                    AssignIfExist(readOnly_, value, GetReadOnlyString());
                if (spec_.oapi >= kVersion30)
                    AssignIfExist(writeOnly_, value, GetWriteOnlyString());
                if (readOnly_ && writeOnly_)
                    schemaDocument->SchemaError(kSchemaErrorReadOnlyAndWriteOnly, p);

                // Nullable - open api 3 only
                // If true add 'null' as allowable type
                if (spec_.oapi >= kVersion30) {
                    AssignIfExist(nullable_, value, GetNullableString());
                    if (nullable_)
                        AddType(GetNullString());
                }
            }

            ~Schema() {
                AllocatorType::Free(enum_);
                if (properties_) {
                    for (SizeType i = 0; i < propertyCount_; i++)
                        properties_[i].~Property();
                    AllocatorType::Free(properties_);
                }
                if (patternProperties_) {
                    for (SizeType i = 0; i < patternPropertyCount_; i++)
                        patternProperties_[i].~PatternProperty();
                    AllocatorType::Free(patternProperties_);
                }
                AllocatorType::Free(itemsTuple_);
#if MERAK_JSON_SCHEMA_HAS_REGEX
                if (pattern_) {
                    pattern_->~RegexType();
                    AllocatorType::Free(pattern_);
                }
#endif
            }

            const SValue &GetURI() const {
                return uri_;
            }

            const UriType &GetId() const {
                return id_;
            }

            const Specification &GetSpecification() const {
                return spec_;
            }

            const PointerType &GetPointer() const {
                return pointer_;
            }

            bool BeginValue(Context &context) const {
                MERAK_JSON_SCHEMA_PRINT(Method, "Schema::BeginValue");
                if (context.inArray) {
                    if (uniqueItems_)
                        context.valueUniqueness = true;

                    if (itemsList_)
                        context.valueSchema = itemsList_;
                    else if (itemsTuple_) {
                        if (context.arrayElementIndex < itemsTupleCount_)
                            context.valueSchema = itemsTuple_[context.arrayElementIndex];
                        else if (additionalItemsSchema_)
                            context.valueSchema = additionalItemsSchema_;
                        else if (additionalItems_)
                            context.valueSchema = typeless_;
                        else {
                            context.error_handler.DisallowedItem(context.arrayElementIndex);
                            // Must set valueSchema for when kValidateContinueOnErrorFlag is set, else reports spurious type error
                            context.valueSchema = typeless_;
                            // Must bump arrayElementIndex for when kValidateContinueOnErrorFlag is set
                            context.arrayElementIndex++;
                            MERAK_JSON_INVALID_KEYWORD_RETURN(kValidateErrorAdditionalItems);
                        }
                    } else
                        context.valueSchema = typeless_;

                    context.arrayElementIndex++;
                }
                return true;
            }

            MERAK_JSON_FORCEINLINE bool EndValue(Context &context) const {
                MERAK_JSON_SCHEMA_PRINT(Method, "Schema::EndValue");
                // Only check pattern properties if we have validators
                if (context.patternPropertiesValidatorCount > 0) {
                    bool otherValid = false;
                    SizeType count = context.patternPropertiesValidatorCount;
                    if (context.objectPatternValidatorType != Context::kPatternValidatorOnly)
                        otherValid = context.patternPropertiesValidators[--count]->IsValid();

                    bool patternValid = true;
                    for (SizeType i = 0; i < count; i++)
                        if (!context.patternPropertiesValidators[i]->IsValid()) {
                            patternValid = false;
                            break;
                        }

                    if (context.objectPatternValidatorType == Context::kPatternValidatorOnly) {
                        if (!patternValid) {
                            context.error_handler.PropertyViolations(context.patternPropertiesValidators, count);
                            MERAK_JSON_INVALID_KEYWORD_RETURN(kValidateErrorPatternProperties);
                        }
                    } else if (context.objectPatternValidatorType == Context::kPatternValidatorWithProperty) {
                        if (!patternValid || !otherValid) {
                            context.error_handler.PropertyViolations(context.patternPropertiesValidators, count + 1);
                            MERAK_JSON_INVALID_KEYWORD_RETURN(kValidateErrorPatternProperties);
                        }
                    } else if (!patternValid && !otherValid) { // kPatternValidatorWithAdditionalProperty)
                        context.error_handler.PropertyViolations(context.patternPropertiesValidators, count + 1);
                        MERAK_JSON_INVALID_KEYWORD_RETURN(kValidateErrorPatternProperties);
                    }
                }

                // For enums only check if we have a hasher
                if (enum_ && context.hasher) {
                    const uint64_t h = context.factory.GetHashCode(context.hasher);
                    for (SizeType i = 0; i < enumCount_; i++)
                        if (enum_[i] == h)
                            goto foundEnum;
                    context.error_handler.DisallowedValue(kValidateErrorEnum);
                    MERAK_JSON_INVALID_KEYWORD_RETURN(kValidateErrorEnum);
                    foundEnum:;
                }

                // Only check allOf etc if we have validators
                if (context.validatorCount > 0) {
                    if (allOf_.schemas)
                        for (SizeType i = allOf_.begin; i < allOf_.begin + allOf_.count; i++)
                            if (!context.validators[i]->IsValid()) {
                                context.error_handler.NotAllOf(&context.validators[allOf_.begin], allOf_.count);
                                MERAK_JSON_INVALID_KEYWORD_RETURN(kValidateErrorAllOf);
                            }

                    if (anyOf_.schemas) {
                        for (SizeType i = anyOf_.begin; i < anyOf_.begin + anyOf_.count; i++)
                            if (context.validators[i]->IsValid())
                                goto foundAny;
                        context.error_handler.NoneOf(&context.validators[anyOf_.begin], anyOf_.count);
                        MERAK_JSON_INVALID_KEYWORD_RETURN(kValidateErrorAnyOf);
                        foundAny:;
                    }

                    if (oneOf_.schemas) {
                        bool oneValid = false;
                        SizeType firstMatch = 0;
                        for (SizeType i = oneOf_.begin; i < oneOf_.begin + oneOf_.count; i++)
                            if (context.validators[i]->IsValid()) {
                                if (oneValid) {
                                    context.error_handler.MultipleOneOf(firstMatch, i - oneOf_.begin);
                                    MERAK_JSON_INVALID_KEYWORD_RETURN(kValidateErrorOneOfMatch);
                                } else {
                                    oneValid = true;
                                    firstMatch = i - oneOf_.begin;
                                }
                            }
                        if (!oneValid) {
                            context.error_handler.NotOneOf(&context.validators[oneOf_.begin], oneOf_.count);
                            MERAK_JSON_INVALID_KEYWORD_RETURN(kValidateErrorOneOf);
                        }
                    }

                    if (not_ && context.validators[notValidatorIndex_]->IsValid()) {
                        context.error_handler.Disallowed();
                        MERAK_JSON_INVALID_KEYWORD_RETURN(kValidateErrorNot);
                    }
                }

                return true;
            }

            bool emplace_null(Context &context) const {
                MERAK_JSON_SCHEMA_PRINT(Method, "Schema::emplace_null");
                if (!(type_ & (1 << kNullSchemaType))) {
                    DisallowedType(context, GetNullString());
                    MERAK_JSON_INVALID_KEYWORD_RETURN(kValidateErrorType);
                }
                return CreateParallelValidator(context);
            }

            bool emplace_bool(Context &context, bool b) const {
                MERAK_JSON_SCHEMA_PRINT(Method, "Schema::emplace_bool", b);
                if (!CheckBool(context, b))
                    return false;
                return CreateParallelValidator(context);
            }

            bool emplace_int32(Context &context, int i) const {
                MERAK_JSON_SCHEMA_PRINT(Method, "Schema::emplace_int32", (int64_t) i);
                if (!CheckInt(context, i))
                    return false;
                return CreateParallelValidator(context);
            }

            bool emplace_uint32(Context &context, unsigned u) const {
                MERAK_JSON_SCHEMA_PRINT(Method, "Schema::Uint", (uint64_t) u);
                if (!CheckUint(context, u))
                    return false;
                return CreateParallelValidator(context);
            }

            bool emplace_int64(Context &context, int64_t i) const {
                MERAK_JSON_SCHEMA_PRINT(Method, "Schema::Int64", i);
                if (!CheckInt(context, i))
                    return false;
                return CreateParallelValidator(context);
            }

            bool emplace_uint64(Context &context, uint64_t u) const {
                MERAK_JSON_SCHEMA_PRINT(Method, "Schema::Uint64", u);
                if (!CheckUint(context, u))
                    return false;
                return CreateParallelValidator(context);
            }

            bool emplace_double(Context &context, double d) const {
                MERAK_JSON_SCHEMA_PRINT(Method, "Schema::Double", d);
                if (!(type_ & (1 << kNumberSchemaType))) {
                    DisallowedType(context, GetNumberString());
                    MERAK_JSON_INVALID_KEYWORD_RETURN(kValidateErrorType);
                }

                if (!minimum_.is_null() && !CheckDoubleMinimum(context, d))
                    return false;

                if (!maximum_.is_null() && !CheckDoubleMaximum(context, d))
                    return false;

                if (!multipleOf_.is_null() && !CheckDoubleMultipleOf(context, d))
                    return false;

                return CreateParallelValidator(context);
            }

            bool emplace_string(Context &context, const char_type *str, SizeType length, bool) const {
                MERAK_JSON_SCHEMA_PRINT(Method, "Schema::String", str);
                if (!(type_ & (1 << kStringSchemaType))) {
                    DisallowedType(context, GetStringString());
                    MERAK_JSON_INVALID_KEYWORD_RETURN(kValidateErrorType);
                }

                if (minLength_ != 0 || maxLength_ != SizeType(~0)) {
                    SizeType count;
                    if (internal::CountStringCodePoint<EncodingType>(str, length, &count)) {
                        if (count < minLength_) {
                            context.error_handler.TooShort(str, length, minLength_);
                            MERAK_JSON_INVALID_KEYWORD_RETURN(kValidateErrorMinLength);
                        }
                        if (count > maxLength_) {
                            context.error_handler.TooLong(str, length, maxLength_);
                            MERAK_JSON_INVALID_KEYWORD_RETURN(kValidateErrorMaxLength);
                        }
                    }
                }

                if (pattern_ && !IsPatternMatch(pattern_, str, length)) {
                    context.error_handler.DoesNotMatch(str, length);
                    MERAK_JSON_INVALID_KEYWORD_RETURN(kValidateErrorPattern);
                }

                return CreateParallelValidator(context);
            }

            bool start_object(Context &context) const {
                MERAK_JSON_SCHEMA_PRINT(Method, "Schema::start_object");
                if (!(type_ & (1 << kObjectSchemaType))) {
                    DisallowedType(context, GetObjectString());
                    MERAK_JSON_INVALID_KEYWORD_RETURN(kValidateErrorType);
                }

                if (hasDependencies_ || hasRequired_) {
                    context.propertyExist = static_cast<bool *>(context.factory.MallocState(
                            sizeof(bool) * propertyCount_));
                    std::memset(context.propertyExist, 0, sizeof(bool) * propertyCount_);
                }

                if (patternProperties_) { // pre-allocate schema array
                    SizeType count = patternPropertyCount_ + 1; // extra for valuePatternValidatorType
                    context.patternPropertiesSchemas = static_cast<const SchemaType **>(context.factory.MallocState(
                            sizeof(const SchemaType *) * count));
                    context.patternPropertiesSchemaCount = 0;
                    std::memset(context.patternPropertiesSchemas, 0, sizeof(SchemaType * ) * count);
                }

                return CreateParallelValidator(context);
            }

            bool emplace_key(Context &context, const char_type *str, SizeType len, bool) const {
                MERAK_JSON_SCHEMA_PRINT(Method, "Schema::Key", str);

                if (patternProperties_) {
                    context.patternPropertiesSchemaCount = 0;
                    for (SizeType i = 0; i < patternPropertyCount_; i++)
                        if (patternProperties_[i].pattern && IsPatternMatch(patternProperties_[i].pattern, str, len)) {
                            context.patternPropertiesSchemas[context.patternPropertiesSchemaCount++] = patternProperties_[i].schema;
                            context.valueSchema = typeless_;
                        }
                }

                SizeType index = 0;
                if (FindPropertyIndex(ValueType(str, len).Move(), &index)) {
                    if (context.patternPropertiesSchemaCount > 0) {
                        context.patternPropertiesSchemas[context.patternPropertiesSchemaCount++] = properties_[index].schema;
                        context.valueSchema = typeless_;
                        context.valuePatternValidatorType = Context::kPatternValidatorWithProperty;
                    } else
                        context.valueSchema = properties_[index].schema;

                    if (context.propertyExist)
                        context.propertyExist[index] = true;

                    return true;
                }

                if (additionalPropertiesSchema_) {
                    if (context.patternPropertiesSchemaCount > 0) {
                        context.patternPropertiesSchemas[context.patternPropertiesSchemaCount++] = additionalPropertiesSchema_;
                        context.valueSchema = typeless_;
                        context.valuePatternValidatorType = Context::kPatternValidatorWithAdditionalProperty;
                    } else
                        context.valueSchema = additionalPropertiesSchema_;
                    return true;
                } else if (additionalProperties_) {
                    context.valueSchema = typeless_;
                    return true;
                }

                if (context.patternPropertiesSchemaCount == 0) { // patternProperties are not additional properties
                    // Must set valueSchema for when kValidateContinueOnErrorFlag is set, else reports spurious type error
                    context.valueSchema = typeless_;
                    context.error_handler.DisallowedProperty(str, len);
                    MERAK_JSON_INVALID_KEYWORD_RETURN(kValidateErrorAdditionalProperties);
                }

                return true;
            }

            bool end_object(Context &context, SizeType memberCount) const {
                MERAK_JSON_SCHEMA_PRINT(Method, "Schema::end_object");
                if (hasRequired_) {
                    context.error_handler.StartMissingProperties();
                    for (SizeType index = 0; index < propertyCount_; index++)
                        if (properties_[index].required && !context.propertyExist[index])
                            if (properties_[index].schema->defaultValueLength_ == 0)
                                context.error_handler.AddMissingProperty(properties_[index].name);
                    if (context.error_handler.EndMissingProperties())
                        MERAK_JSON_INVALID_KEYWORD_RETURN(kValidateErrorRequired);
                }

                if (memberCount < minProperties_) {
                    context.error_handler.TooFewProperties(memberCount, minProperties_);
                    MERAK_JSON_INVALID_KEYWORD_RETURN(kValidateErrorMinProperties);
                }

                if (memberCount > maxProperties_) {
                    context.error_handler.TooManyProperties(memberCount, maxProperties_);
                    MERAK_JSON_INVALID_KEYWORD_RETURN(kValidateErrorMaxProperties);
                }

                if (hasDependencies_) {
                    context.error_handler.StartDependencyErrors();
                    for (SizeType sourceIndex = 0; sourceIndex < propertyCount_; sourceIndex++) {
                        const Property &source = properties_[sourceIndex];
                        if (context.propertyExist[sourceIndex]) {
                            if (source.dependencies) {
                                context.error_handler.StartMissingDependentProperties();
                                for (SizeType targetIndex = 0; targetIndex < propertyCount_; targetIndex++)
                                    if (source.dependencies[targetIndex] && !context.propertyExist[targetIndex])
                                        context.error_handler.AddMissingDependentProperty(
                                                properties_[targetIndex].name);
                                context.error_handler.EndMissingDependentProperties(source.name);
                            } else if (source.dependenciesSchema) {
                                ISchemaValidator *dependenciesValidator = context.validators[source.dependenciesValidatorIndex];
                                if (!dependenciesValidator->IsValid())
                                    context.error_handler.AddDependencySchemaError(source.name, dependenciesValidator);
                            }
                        }
                    }
                    if (context.error_handler.EndDependencyErrors())
                        MERAK_JSON_INVALID_KEYWORD_RETURN(kValidateErrorDependencies);
                }

                return true;
            }

            bool start_array(Context &context) const {
                MERAK_JSON_SCHEMA_PRINT(Method, "Schema::start_array");
                context.arrayElementIndex = 0;
                context.inArray = true;  // Ensure we note that we are in an array

                if (!(type_ & (1 << kArraySchemaType))) {
                    DisallowedType(context, GetArrayString());
                    MERAK_JSON_INVALID_KEYWORD_RETURN(kValidateErrorType);
                }

                return CreateParallelValidator(context);
            }

            bool end_array(Context &context, SizeType elementCount) const {
                MERAK_JSON_SCHEMA_PRINT(Method, "Schema::end_array");
                context.inArray = false;

                if (elementCount < minItems_) {
                    context.error_handler.TooFewItems(elementCount, minItems_);
                    MERAK_JSON_INVALID_KEYWORD_RETURN(kValidateErrorMinItems);
                }

                if (elementCount > maxItems_) {
                    context.error_handler.TooManyItems(elementCount, maxItems_);
                    MERAK_JSON_INVALID_KEYWORD_RETURN(kValidateErrorMaxItems);
                }

                return true;
            }

            static const ValueType &GetValidateErrorKeyword(ValidateErrorCode validateErrorCode) {
                switch (validateErrorCode) {
                    case kValidateErrorMultipleOf:
                        return GetMultipleOfString();
                    case kValidateErrorMaximum:
                        return GetMaximumString();
                    case kValidateErrorExclusiveMaximum:
                        return GetMaximumString(); // Same
                    case kValidateErrorMinimum:
                        return GetMinimumString();
                    case kValidateErrorExclusiveMinimum:
                        return GetMinimumString(); // Same

                    case kValidateErrorMaxLength:
                        return GetMaxLengthString();
                    case kValidateErrorMinLength:
                        return GetMinLengthString();
                    case kValidateErrorPattern:
                        return GetPatternString();

                    case kValidateErrorMaxItems:
                        return GetMaxItemsString();
                    case kValidateErrorMinItems:
                        return GetMinItemsString();
                    case kValidateErrorUniqueItems:
                        return GetUniqueItemsString();
                    case kValidateErrorAdditionalItems:
                        return GetAdditionalItemsString();

                    case kValidateErrorMaxProperties:
                        return GetMaxPropertiesString();
                    case kValidateErrorMinProperties:
                        return GetMinPropertiesString();
                    case kValidateErrorRequired:
                        return GetRequiredString();
                    case kValidateErrorAdditionalProperties:
                        return GetAdditionalPropertiesString();
                    case kValidateErrorPatternProperties:
                        return GetPatternPropertiesString();
                    case kValidateErrorDependencies:
                        return GetDependenciesString();

                    case kValidateErrorEnum:
                        return GetEnumString();
                    case kValidateErrorType:
                        return GetTypeString();

                    case kValidateErrorOneOf:
                        return GetOneOfString();
                    case kValidateErrorOneOfMatch:
                        return GetOneOfString(); // Same
                    case kValidateErrorAllOf:
                        return GetAllOfString();
                    case kValidateErrorAnyOf:
                        return GetAnyOfString();
                    case kValidateErrorNot:
                        return GetNotString();

                    case kValidateErrorReadOnly:
                        return GetReadOnlyString();
                    case kValidateErrorWriteOnly:
                        return GetWriteOnlyString();

                    default:
                        return GetNullString();
                }
            }


            // Generate functions for string literal according to char_type
#define MERAK_JSON_STRING_(name, ...) \
        static const ValueType& Get##name##String() {\
            static const char_type s[] = { __VA_ARGS__, '\0' };\
            static const ValueType v(s, static_cast<SizeType>(sizeof(s) / sizeof(char_type) - 1));\
            return v;\
        }

            MERAK_JSON_STRING_(Null, 'n', 'u', 'l', 'l')

            MERAK_JSON_STRING_(Boolean, 'b', 'o', 'o', 'l', 'e', 'a', 'n')

            MERAK_JSON_STRING_(Object, 'o', 'b', 'j', 'e', 'c', 't')

            MERAK_JSON_STRING_(Array, 'a', 'r', 'r', 'a', 'y')

            MERAK_JSON_STRING_(String, 's', 't', 'r', 'i', 'n', 'g')

            MERAK_JSON_STRING_(Number, 'n', 'u', 'm', 'b', 'e', 'r')

            MERAK_JSON_STRING_(Integer, 'i', 'n', 't', 'e', 'g', 'e', 'r')

            MERAK_JSON_STRING_(Type, 't', 'y', 'p', 'e')

            MERAK_JSON_STRING_(Enum, 'e', 'n', 'u', 'm')

            MERAK_JSON_STRING_(AllOf, 'a', 'l', 'l', 'O', 'f')

            MERAK_JSON_STRING_(AnyOf, 'a', 'n', 'y', 'O', 'f')

            MERAK_JSON_STRING_(OneOf, 'o', 'n', 'e', 'O', 'f')

            MERAK_JSON_STRING_(Not, 'n', 'o', 't')

            MERAK_JSON_STRING_(Properties, 'p', 'r', 'o', 'p', 'e', 'r', 't', 'i', 'e', 's')

            MERAK_JSON_STRING_(Required, 'r', 'e', 'q', 'u', 'i', 'r', 'e', 'd')

            MERAK_JSON_STRING_(Dependencies, 'd', 'e', 'p', 'e', 'n', 'd', 'e', 'n', 'c', 'i', 'e', 's')

            MERAK_JSON_STRING_(PatternProperties, 'p', 'a', 't', 't', 'e', 'r', 'n', 'P', 'r', 'o', 'p', 'e', 'r', 't',
                               'i', 'e', 's')

            MERAK_JSON_STRING_(AdditionalProperties, 'a', 'd', 'd', 'i', 't', 'i', 'o', 'n', 'a', 'l', 'P', 'r', 'o',
                               'p', 'e', 'r', 't', 'i', 'e', 's')

            MERAK_JSON_STRING_(MinProperties, 'm', 'i', 'n', 'P', 'r', 'o', 'p', 'e', 'r', 't', 'i', 'e', 's')

            MERAK_JSON_STRING_(MaxProperties, 'm', 'a', 'x', 'P', 'r', 'o', 'p', 'e', 'r', 't', 'i', 'e', 's')

            MERAK_JSON_STRING_(Items, 'i', 't', 'e', 'm', 's')

            MERAK_JSON_STRING_(MinItems, 'm', 'i', 'n', 'I', 't', 'e', 'm', 's')

            MERAK_JSON_STRING_(MaxItems, 'm', 'a', 'x', 'I', 't', 'e', 'm', 's')

            MERAK_JSON_STRING_(AdditionalItems, 'a', 'd', 'd', 'i', 't', 'i', 'o', 'n', 'a', 'l', 'I', 't', 'e', 'm',
                               's')

            MERAK_JSON_STRING_(UniqueItems, 'u', 'n', 'i', 'q', 'u', 'e', 'I', 't', 'e', 'm', 's')

            MERAK_JSON_STRING_(MinLength, 'm', 'i', 'n', 'L', 'e', 'n', 'g', 't', 'h')

            MERAK_JSON_STRING_(MaxLength, 'm', 'a', 'x', 'L', 'e', 'n', 'g', 't', 'h')

            MERAK_JSON_STRING_(Pattern, 'p', 'a', 't', 't', 'e', 'r', 'n')

            MERAK_JSON_STRING_(Minimum, 'm', 'i', 'n', 'i', 'm', 'u', 'm')

            MERAK_JSON_STRING_(Maximum, 'm', 'a', 'x', 'i', 'm', 'u', 'm')

            MERAK_JSON_STRING_(ExclusiveMinimum, 'e', 'x', 'c', 'l', 'u', 's', 'i', 'v', 'e', 'M', 'i', 'n', 'i', 'm',
                               'u', 'm')

            MERAK_JSON_STRING_(ExclusiveMaximum, 'e', 'x', 'c', 'l', 'u', 's', 'i', 'v', 'e', 'M', 'a', 'x', 'i', 'm',
                               'u', 'm')

            MERAK_JSON_STRING_(MultipleOf, 'm', 'u', 'l', 't', 'i', 'p', 'l', 'e', 'O', 'f')

            MERAK_JSON_STRING_(DefaultValue, 'd', 'e', 'f', 'a', 'u', 'l', 't')

            MERAK_JSON_STRING_(Schema, '$', 's', 'c', 'h', 'e', 'm', 'a')

            MERAK_JSON_STRING_(Ref, '$', 'r', 'e', 'f')

            MERAK_JSON_STRING_(Id, 'i', 'd')

            MERAK_JSON_STRING_(Swagger, 's', 'w', 'a', 'g', 'g', 'e', 'r')

            MERAK_JSON_STRING_(OpenApi, 'o', 'p', 'e', 'n', 'a', 'p', 'i')

            MERAK_JSON_STRING_(ReadOnly, 'r', 'e', 'a', 'd', 'O', 'n', 'l', 'y')

            MERAK_JSON_STRING_(WriteOnly, 'w', 'r', 'i', 't', 'e', 'O', 'n', 'l', 'y')

            MERAK_JSON_STRING_(Nullable, 'n', 'u', 'l', 'l', 'a', 'b', 'l', 'e')

#undef MERAK_JSON_STRING_

        private:
            enum SchemaValueType {
                kNullSchemaType,
                kBooleanSchemaType,
                kObjectSchemaType,
                kArraySchemaType,
                kStringSchemaType,
                kNumberSchemaType,
                kIntegerSchemaType,
                kTotalSchemaType
            };

#if MERAK_JSON_SCHEMA_USE_INTERNALREGEX
            typedef internal::GenericRegex<EncodingType, AllocatorType> RegexType;
#elif MERAK_JSON_SCHEMA_USE_STDREGEX
            typedef std::basic_regex<char_type> RegexType;
#else
            typedef char RegexType;
#endif

            struct SchemaArray {
                SchemaArray() : schemas(), count() {}

                ~SchemaArray() { AllocatorType::Free(schemas); }

                const SchemaType **schemas;
                SizeType begin; // begin index of context.validators
                SizeType count;
            };

            template<typename V1, typename V2>
            void AddUniqueElement(V1 &a, const V2 &v) {
                for (typename V1::ConstValueIterator itr = a.begin(); itr != a.end(); ++itr)
                    if (*itr == v)
                        return;
                V1 c(v, *allocator_);
                a.push_back(c, *allocator_);
            }

            static const ValueType *GetMember(const ValueType &value, const ValueType &name) {
                typename ValueType::ConstMemberIterator itr = value.find_member(name);
                return itr != value.member_end() ? &(itr->value) : 0;
            }

            static void AssignIfExist(bool &out, const ValueType &value, const ValueType &name) {
                if (const ValueType *v = GetMember(value, name))
                    if (v->is_bool())
                        out = v->get_bool();
            }

            static void AssignIfExist(SizeType &out, const ValueType &value, const ValueType &name) {
                if (const ValueType *v = GetMember(value, name))
                    if (v->is_uint64() && v->get_uint64() <= SizeType(~0))
                        out = static_cast<SizeType>(v->get_uint64());
            }

            void AssignIfExist(SchemaArray &out, SchemaDocumentType &schemaDocument, const PointerType &p,
                               const ValueType &value, const ValueType &name, const ValueType &document) {
                if (const ValueType *v = GetMember(value, name)) {
                    if (v->is_array() && v->size() > 0) {
                        PointerType q = p.Append(name, allocator_);
                        out.count = v->size();
                        out.schemas = static_cast<const Schema **>(allocator_->Malloc(
                                out.count * sizeof(const Schema *)));
                        memset(out.schemas, 0, sizeof(Schema *) * out.count);
                        for (SizeType i = 0; i < out.count; i++)
                            schemaDocument.CreateSchema(&out.schemas[i], q.Append(i, allocator_), (*v)[i], document,
                                                        id_);
                        out.begin = validatorCount_;
                        validatorCount_ += out.count;
                    }
                }
            }

#if MERAK_JSON_SCHEMA_USE_INTERNALREGEX

            template<typename ValueType>
            RegexType *CreatePattern(const ValueType &value, SchemaDocumentType *sd, const PointerType &p) {
                if (value.is_string()) {
                    RegexType *r = new(allocator_->Malloc(sizeof(RegexType))) RegexType(value.get_string(), allocator_);
                    if (!r->IsValid()) {
                        sd->SchemaErrorValue(kSchemaErrorRegexInvalid, p, value.get_string(), value.get_string_length());
                        r->~RegexType();
                        AllocatorType::Free(r);
                        r = 0;
                    }
                    return r;
                }
                return 0;
            }

            static bool IsPatternMatch(const RegexType *pattern, const char_type *str, SizeType) {
                GenericRegexSearch<RegexType> rs(*pattern);
                return rs.Search(str);
            }

#elif MERAK_JSON_SCHEMA_USE_STDREGEX
            template <typename ValueType>
            RegexType* CreatePattern(const ValueType& value, SchemaDocumentType* sd, const PointerType& p) {
                if (value.is_string()) {
                    RegexType *r = static_cast<RegexType*>(allocator_->Malloc(sizeof(RegexType)));
                    try {
                        return new (r) RegexType(value.get_string(), std::size_t(value.get_string_length()), std::regex_constants::ECMAScript);
                    }
                    catch (const std::regex_error& e) {
                        sd->SchemaErrorValue(kSchemaErrorRegexInvalid, p, value.get_string(), value.get_string_length());
                        AllocatorType::Free(r);
                    }
                }
                return 0;
            }

            static bool IsPatternMatch(const RegexType* pattern, const char_type *str, SizeType length) {
                std::match_results<const char_type*> r;
                return std::regex_search(str, str + length, r, *pattern);
            }
#else
            template <typename ValueType>
            RegexType* CreatePattern(const ValueType&) {
                return 0;
            }

            static bool IsPatternMatch(const RegexType*, const char_type *, SizeType) { return true; }
#endif // MERAK_JSON_SCHEMA_USE_STDREGEX

            void AddType(const ValueType &type) {
                if (type == GetNullString()) type_ |= 1 << kNullSchemaType;
                else if (type == GetBooleanString()) type_ |= 1 << kBooleanSchemaType;
                else if (type == GetObjectString()) type_ |= 1 << kObjectSchemaType;
                else if (type == GetArrayString()) type_ |= 1 << kArraySchemaType;
                else if (type == GetStringString()) type_ |= 1 << kStringSchemaType;
                else if (type == GetIntegerString()) type_ |= 1 << kIntegerSchemaType;
                else if (type == GetNumberString()) type_ |= (1 << kNumberSchemaType) | (1 << kIntegerSchemaType);
            }

            // Creates parallel validators for allOf, anyOf, oneOf, not and schema dependencies, if required.
            // Also creates a hasher for enums and array uniqueness, if required.
            // Also a useful place to add type-independent error checks.
            bool CreateParallelValidator(Context &context) const {
                if (enum_ || context.arrayUniqueness)
                    context.hasher = context.factory.CreateHasher();

                if (validatorCount_) {
                    MERAK_JSON_ASSERT(context.validators == 0);
                    context.validators = static_cast<ISchemaValidator **>(context.factory.MallocState(
                            sizeof(ISchemaValidator *) * validatorCount_));
                    std::memset(context.validators, 0, sizeof(ISchemaValidator *) * validatorCount_);
                    context.validatorCount = validatorCount_;

                    // Always return after first failure for these sub-validators
                    if (allOf_.schemas)
                        CreateSchemaValidators(context, allOf_, false);

                    if (anyOf_.schemas)
                        CreateSchemaValidators(context, anyOf_, false);

                    if (oneOf_.schemas)
                        CreateSchemaValidators(context, oneOf_, false);

                    if (not_)
                        context.validators[notValidatorIndex_] = context.factory.CreateSchemaValidator(*not_, false);

                    if (hasSchemaDependencies_) {
                        for (SizeType i = 0; i < propertyCount_; i++)
                            if (properties_[i].dependenciesSchema)
                                context.validators[properties_[i].dependenciesValidatorIndex] = context.factory.CreateSchemaValidator(
                                        *properties_[i].dependenciesSchema, false);
                    }
                }

                // Add any other type-independent checks here
                if (readOnly_ && (context.flags & kValidateWriteFlag)) {
                    context.error_handler.DisallowedWhenWriting();
                    MERAK_JSON_INVALID_KEYWORD_RETURN(kValidateErrorReadOnly);
                }
                if (writeOnly_ && (context.flags & kValidateReadFlag)) {
                    context.error_handler.DisallowedWhenReading();
                    MERAK_JSON_INVALID_KEYWORD_RETURN(kValidateErrorWriteOnly);
                }

                return true;
            }

            void CreateSchemaValidators(Context &context, const SchemaArray &schemas,
                                        const bool inheritContinueOnErrors) const {
                for (SizeType i = 0; i < schemas.count; i++)
                    context.validators[schemas.begin + i] = context.factory.CreateSchemaValidator(*schemas.schemas[i],
                                                                                                  inheritContinueOnErrors);
            }

            // O(n)
            bool FindPropertyIndex(const ValueType &name, SizeType *outIndex) const {
                SizeType len = name.get_string_length();
                const char_type *str = name.get_string();
                for (SizeType index = 0; index < propertyCount_; index++)
                    if (properties_[index].name.get_string_length() == len &&
                        (std::memcmp(properties_[index].name.get_string(), str, sizeof(char_type) * len) == 0)) {
                        *outIndex = index;
                        return true;
                    }
                return false;
            }

            bool CheckBool(Context &context, bool) const {
                if (!(type_ & (1 << kBooleanSchemaType))) {
                    DisallowedType(context, GetBooleanString());
                    MERAK_JSON_INVALID_KEYWORD_RETURN(kValidateErrorType);
                }
                return true;
            }

            bool CheckInt(Context &context, int64_t i) const {
                if (!(type_ & ((1 << kIntegerSchemaType) | (1 << kNumberSchemaType)))) {
                    DisallowedType(context, GetIntegerString());
                    MERAK_JSON_INVALID_KEYWORD_RETURN(kValidateErrorType);
                }

                if (!minimum_.is_null()) {
                    if (minimum_.is_int64()) {
                        if (exclusiveMinimum_ ? i <= minimum_.get_int64() : i < minimum_.get_int64()) {
                            context.error_handler.BelowMinimum(i, minimum_, exclusiveMinimum_);
                            MERAK_JSON_INVALID_KEYWORD_RETURN(
                                    exclusiveMinimum_ ? kValidateErrorExclusiveMinimum : kValidateErrorMinimum);
                        }
                    } else if (minimum_.is_uint64()) {
                        context.error_handler.BelowMinimum(i, minimum_, exclusiveMinimum_);
                        MERAK_JSON_INVALID_KEYWORD_RETURN(exclusiveMinimum_ ? kValidateErrorExclusiveMinimum
                                                                            : kValidateErrorMinimum); // i <= max(int64_t) < minimum.get_uint64()
                    } else if (!CheckDoubleMinimum(context, static_cast<double>(i)))
                        return false;
                }

                if (!maximum_.is_null()) {
                    if (maximum_.is_int64()) {
                        if (exclusiveMaximum_ ? i >= maximum_.get_int64() : i > maximum_.get_int64()) {
                            context.error_handler.AboveMaximum(i, maximum_, exclusiveMaximum_);
                            MERAK_JSON_INVALID_KEYWORD_RETURN(
                                    exclusiveMaximum_ ? kValidateErrorExclusiveMaximum : kValidateErrorMaximum);
                        }
                    } else if (maximum_.is_uint64()) {}
                        /* do nothing */ // i <= max(int64_t) < maximum_.get_uint64()
                    else if (!CheckDoubleMaximum(context, static_cast<double>(i)))
                        return false;
                }

                if (!multipleOf_.is_null()) {
                    if (multipleOf_.is_uint64()) {
                        if (static_cast<uint64_t>(i >= 0 ? i : -i) % multipleOf_.get_uint64() != 0) {
                            context.error_handler.NotMultipleOf(i, multipleOf_);
                            MERAK_JSON_INVALID_KEYWORD_RETURN(kValidateErrorMultipleOf);
                        }
                    } else if (!CheckDoubleMultipleOf(context, static_cast<double>(i)))
                        return false;
                }

                return true;
            }

            bool CheckUint(Context &context, uint64_t i) const {
                if (!(type_ & ((1 << kIntegerSchemaType) | (1 << kNumberSchemaType)))) {
                    DisallowedType(context, GetIntegerString());
                    MERAK_JSON_INVALID_KEYWORD_RETURN(kValidateErrorType);
                }

                if (!minimum_.is_null()) {
                    if (minimum_.is_uint64()) {
                        if (exclusiveMinimum_ ? i <= minimum_.get_uint64() : i < minimum_.get_uint64()) {
                            context.error_handler.BelowMinimum(i, minimum_, exclusiveMinimum_);
                            MERAK_JSON_INVALID_KEYWORD_RETURN(
                                    exclusiveMinimum_ ? kValidateErrorExclusiveMinimum : kValidateErrorMinimum);
                        }
                    } else if (minimum_.is_int64())
                        /* do nothing */; // i >= 0 > minimum.Getint64()
                    else if (!CheckDoubleMinimum(context, static_cast<double>(i)))
                        return false;
                }

                if (!maximum_.is_null()) {
                    if (maximum_.is_uint64()) {
                        if (exclusiveMaximum_ ? i >= maximum_.get_uint64() : i > maximum_.get_uint64()) {
                            context.error_handler.AboveMaximum(i, maximum_, exclusiveMaximum_);
                            MERAK_JSON_INVALID_KEYWORD_RETURN(
                                    exclusiveMaximum_ ? kValidateErrorExclusiveMaximum : kValidateErrorMaximum);
                        }
                    } else if (maximum_.is_int64()) {
                        context.error_handler.AboveMaximum(i, maximum_, exclusiveMaximum_);
                        MERAK_JSON_INVALID_KEYWORD_RETURN(exclusiveMaximum_ ? kValidateErrorExclusiveMaximum
                                                                            : kValidateErrorMaximum); // i >= 0 > maximum_
                    } else if (!CheckDoubleMaximum(context, static_cast<double>(i)))
                        return false;
                }

                if (!multipleOf_.is_null()) {
                    if (multipleOf_.is_uint64()) {
                        if (i % multipleOf_.get_uint64() != 0) {
                            context.error_handler.NotMultipleOf(i, multipleOf_);
                            MERAK_JSON_INVALID_KEYWORD_RETURN(kValidateErrorMultipleOf);
                        }
                    } else if (!CheckDoubleMultipleOf(context, static_cast<double>(i)))
                        return false;
                }

                return true;
            }

            bool CheckDoubleMinimum(Context &context, double d) const {
                if (exclusiveMinimum_ ? d <= minimum_.get_double() : d < minimum_.get_double()) {
                    context.error_handler.BelowMinimum(d, minimum_, exclusiveMinimum_);
                    MERAK_JSON_INVALID_KEYWORD_RETURN(
                            exclusiveMinimum_ ? kValidateErrorExclusiveMinimum : kValidateErrorMinimum);
                }
                return true;
            }

            bool CheckDoubleMaximum(Context &context, double d) const {
                if (exclusiveMaximum_ ? d >= maximum_.get_double() : d > maximum_.get_double()) {
                    context.error_handler.AboveMaximum(d, maximum_, exclusiveMaximum_);
                    MERAK_JSON_INVALID_KEYWORD_RETURN(
                            exclusiveMaximum_ ? kValidateErrorExclusiveMaximum : kValidateErrorMaximum);
                }
                return true;
            }

            bool CheckDoubleMultipleOf(Context &context, double d) const {
                double a = std::abs(d), b = std::abs(multipleOf_.get_double());
                double q = a / b;
                double qRounded = std::floor(q + 0.5);
                double scaledEpsilon = (q + qRounded) * std::numeric_limits<double>::epsilon();
                double difference = std::abs(qRounded - q);
                bool isMultiple = difference <= scaledEpsilon || difference < (std::numeric_limits<double>::min)();
                if (!isMultiple) {
                    context.error_handler.NotMultipleOf(d, multipleOf_);
                    MERAK_JSON_INVALID_KEYWORD_RETURN(kValidateErrorMultipleOf);
                }
                return true;
            }

            void DisallowedType(Context &context, const ValueType &actualType) const {
                ErrorHandler &eh = context.error_handler;
                eh.StartDisallowedType();

                if (type_ & (1 << kNullSchemaType)) eh.AddExpectedType(GetNullString());
                if (type_ & (1 << kBooleanSchemaType)) eh.AddExpectedType(GetBooleanString());
                if (type_ & (1 << kObjectSchemaType)) eh.AddExpectedType(GetObjectString());
                if (type_ & (1 << kArraySchemaType)) eh.AddExpectedType(GetArrayString());
                if (type_ & (1 << kStringSchemaType)) eh.AddExpectedType(GetStringString());

                if (type_ & (1 << kNumberSchemaType)) eh.AddExpectedType(GetNumberString());
                else if (type_ & (1 << kIntegerSchemaType)) eh.AddExpectedType(GetIntegerString());

                eh.EndDisallowedType(actualType);
            }

            struct Property {
                Property()
                        : schema(), dependenciesSchema(), dependenciesValidatorIndex(), dependencies(),
                          required(false) {}

                ~Property() { AllocatorType::Free(dependencies); }

                SValue name;
                const SchemaType *schema;
                const SchemaType *dependenciesSchema;
                SizeType dependenciesValidatorIndex;
                bool *dependencies;
                bool required;
            };

            struct PatternProperty {
                PatternProperty() : schema(), pattern() {}

                ~PatternProperty() {
                    if (pattern) {
                        pattern->~RegexType();
                        AllocatorType::Free(pattern);
                    }
                }

                const SchemaType *schema;
                RegexType *pattern;
            };

            AllocatorType *allocator_;
            SValue uri_;
            UriType id_;
            Specification spec_;
            PointerType pointer_;
            const SchemaType *typeless_;
            uint64_t *enum_;
            SizeType enumCount_;
            SchemaArray allOf_;
            SchemaArray anyOf_;
            SchemaArray oneOf_;
            const SchemaType *not_;
            unsigned type_; // bitmask of kSchemaType
            SizeType validatorCount_;
            SizeType notValidatorIndex_;

            Property *properties_;
            const SchemaType *additionalPropertiesSchema_;
            PatternProperty *patternProperties_;
            SizeType patternPropertyCount_;
            SizeType propertyCount_;
            SizeType minProperties_;
            SizeType maxProperties_;
            bool additionalProperties_;
            bool hasDependencies_;
            bool hasRequired_;
            bool hasSchemaDependencies_;

            const SchemaType *additionalItemsSchema_;
            const SchemaType *itemsList_;
            const SchemaType **itemsTuple_;
            SizeType itemsTupleCount_;
            SizeType minItems_;
            SizeType maxItems_;
            bool additionalItems_;
            bool uniqueItems_;

            RegexType *pattern_;
            SizeType minLength_;
            SizeType maxLength_;

            SValue minimum_;
            SValue maximum_;
            SValue multipleOf_;
            bool exclusiveMinimum_;
            bool exclusiveMaximum_;

            SizeType defaultValueLength_;

            bool readOnly_;
            bool writeOnly_;
            bool nullable_;
        };

        template<typename Stack, typename char_type>
        struct TokenHelper {
            MERAK_JSON_FORCEINLINE static void AppendIndexToken(Stack &documentStack, SizeType index) {
                *documentStack.template Push<char_type>() = '/';
                char buffer[21];
                size_t length = static_cast<size_t>(
                        (sizeof(SizeType) == 4 ? u32toa(index, buffer) : u64toa(index, buffer)) - buffer);
                for (size_t i = 0; i < length; i++)
                    *documentStack.template Push<char_type>() = static_cast<char_type>(buffer[i]);
            }
        };

        // Partial specialized version for char to prevent buffer copying.
        template<typename Stack>
        struct TokenHelper<Stack, char> {
            MERAK_JSON_FORCEINLINE static void AppendIndexToken(Stack &documentStack, SizeType index) {
                if constexpr (sizeof(SizeType) == 4) {
                    char *buffer = documentStack.template Push<char>(1 + 10); // '/' + uint
                    *buffer++ = '/';
                    const char *end = internal::u32toa(index, buffer);
                    documentStack.template Pop<char>(static_cast<size_t>(10 - (end - buffer)));
                } else {
                    char *buffer = documentStack.template Push<char>(1 + 20); // '/' + uint64
                    *buffer++ = '/';
                    const char *end = internal::u64toa(index, buffer);
                    documentStack.template Pop<char>(static_cast<size_t>(20 - (end - buffer)));
                }
            }
        };

    } // namespace internal

    ///////////////////////////////////////////////////////////////////////////////
    // IGenericRemoteSchemaDocumentProvider

    template<typename SchemaDocumentType>
    class IGenericRemoteSchemaDocumentProvider {
    public:
        typedef typename SchemaDocumentType::char_type char_type;
        typedef typename SchemaDocumentType::ValueType ValueType;
        typedef typename SchemaDocumentType::AllocatorType AllocatorType;

        virtual ~IGenericRemoteSchemaDocumentProvider() {}

        virtual const SchemaDocumentType *GetRemoteDocument(const char_type *uri, SizeType length) = 0;

        virtual const SchemaDocumentType *
        GetRemoteDocument(const GenericUri<ValueType, AllocatorType> uri, Specification &spec) {
            // Default implementation just calls through for compatibility
            // Following line suppresses unused parameter warning
            (void) spec;
            // printf("GetRemoteDocument: %d %d\n", spec.draft, spec.oapi);
            return GetRemoteDocument(uri.GetBaseString(), uri.GetBaseStringLength());
        }
    };

    ///////////////////////////////////////////////////////////////////////////////
    // GenericSchemaDocument

    //! JSON schema document.
    /*!
        A JSON schema document is a compiled version of a JSON schema.
        It is basically a tree of internal::Schema.

        \note This is an immutable class (i.e. its instance cannot be modified after construction).
        \tparam ValueT Type of JSON value (e.g. \c Value ), which also determine the encoding.
        \tparam Allocator Allocator type for allocating memory of this document.
    */
    template<typename ValueT, typename Allocator = CrtAllocator>
    class GenericSchemaDocument {
    public:
        typedef ValueT ValueType;
        typedef IGenericRemoteSchemaDocumentProvider<GenericSchemaDocument> IRemoteSchemaDocumentProviderType;
        typedef Allocator AllocatorType;
        typedef typename ValueType::EncodingType EncodingType;
        typedef typename EncodingType::char_type char_type;
        typedef internal::Schema<GenericSchemaDocument> SchemaType;
        typedef GenericPointer<ValueType, Allocator> PointerType;
        typedef GenericValue<EncodingType, AllocatorType> GValue;
        typedef GenericUri<ValueType, Allocator> UriType;
        typedef GenericStringRef<char_type> StringRefType;

        friend class internal::Schema<GenericSchemaDocument>;

        template<typename, typename, typename>
        friend
        class GenericSchemaValidator;

        //! Constructor.
        /*!
            Compile a JSON document into schema document.

            \param document A JSON document as source.
            \param uri The base URI of this schema document for purposes of violation reporting.
            \param uriLength Length of \c name, in code points.
            \param remoteProvider An optional remote schema document provider for resolving remote reference. Can be null.
            \param allocator An optional allocator instance for allocating memory. Can be null.
            \param pointer An optional JSON pointer to the start of the schema document
            \param spec Optional schema draft or OpenAPI version. Used if no specification in document. Defaults to draft-04.
        */
        explicit GenericSchemaDocument(const ValueType &document, const char_type *uri = 0, SizeType uriLength = 0,
                                       IRemoteSchemaDocumentProviderType *remoteProvider = 0, Allocator *allocator = 0,
                                       const PointerType &pointer = PointerType(), // PR #1393
                                       const Specification &spec = Specification(kDraft04)) :
                remoteProvider_(remoteProvider),
                allocator_(allocator),
                ownAllocator_(),
                root_(),
                typeless_(),
                schemaMap_(allocator, kInitialSchemaMapSize),
                schemaRef_(allocator, kInitialSchemaRefSize),
                spec_(spec),
                error_(kObjectType),
                currentError_() {
            MERAK_JSON_SCHEMA_PRINT(Method, "GenericSchemaDocument::GenericSchemaDocument");
            if (!allocator_)
                ownAllocator_ = allocator_ = MERAK_JSON_NEW(Allocator)();

            char_type noUri[1] = {0};
            uri_.set_string(uri ? uri : noUri, uriLength, *allocator_);
            docId_ = UriType(uri_, allocator_);

            typeless_ = static_cast<SchemaType *>(allocator_->Malloc(sizeof(SchemaType)));
            new(typeless_) SchemaType(this, PointerType(), ValueType(kObjectType).Move(), ValueType(kObjectType).Move(),
                                      allocator_, docId_);

            // Establish the schema draft or open api version.
            // We only ever look for '$schema' or 'swagger' or 'openapi' at the root of the document.
            SetSchemaSpecification(document);

            // Generate root schema, it will call CreateSchema() to create sub-schemas,
            // And call HandleRefSchema() if there are $ref.
            // PR #1393 use input pointer if supplied
            root_ = typeless_;
            if (pointer.GetTokenCount() == 0) {
                CreateSchemaRecursive(&root_, pointer, document, document, docId_);
            } else if (const ValueType *v = pointer.get(document)) {
                CreateSchema(&root_, pointer, *v, document, docId_);
            } else {
                GenericStringBuffer<EncodingType> sb;
                pointer.StringifyUriFragment(sb);
                SchemaErrorValue(kSchemaErrorStartUnknown, PointerType(), sb.get_string(),
                                 static_cast<SizeType>(sb.GetSize() / sizeof(char_type)));
            }

            MERAK_JSON_ASSERT(root_ != 0);

            schemaRef_.ShrinkToFit(); // Deallocate all memory for ref
        }

        //! Move constructor in C++11
        GenericSchemaDocument(GenericSchemaDocument &&rhs) MERAK_JSON_NOEXCEPT:
                remoteProvider_(rhs.remoteProvider_),
                allocator_(rhs.allocator_),
                ownAllocator_(rhs.ownAllocator_),
                root_(rhs.root_),
                typeless_(rhs.typeless_),
                schemaMap_(std::move(rhs.schemaMap_)),
                schemaRef_(std::move(rhs.schemaRef_)),
                uri_(std::move(rhs.uri_)),
                docId_(std::move(rhs.docId_)),
                spec_(rhs.spec_),
                error_(std::move(rhs.error_)),
                currentError_(std::move(rhs.currentError_)) {
            rhs.remoteProvider_ = 0;
            rhs.allocator_ = 0;
            rhs.ownAllocator_ = 0;
            rhs.typeless_ = 0;
        }

        //! Destructor
        ~GenericSchemaDocument() {
            while (!schemaMap_.empty())
                schemaMap_.template Pop<SchemaEntry>(1)->~SchemaEntry();

            if (typeless_) {
                typeless_->~SchemaType();
                Allocator::Free(typeless_);
            }

            // these may contain some allocator data so clear before deleting ownAllocator_
            uri_.SetNull();
            error_.SetNull();
            currentError_.SetNull();

            MERAK_JSON_DELETE(ownAllocator_);
        }

        const GValue &GetURI() const { return uri_; }

        const Specification &GetSpecification() const { return spec_; }

        bool IsSupportedSpecification() const { return spec_.IsSupported(); }

        //! Static method to get the specification of any schema document
        //  Returns kDraftNone if document is silent
        static const Specification GetSpecification(const ValueType &document) {
            SchemaDraft draft = GetSchemaDraft(document);
            if (draft != kDraftNone)
                return Specification(draft);
            else {
                OpenApiVersion oapi = GetOpenApiVersion(document);
                if (oapi != kVersionNone)
                    return Specification(oapi);
            }
            return Specification(kDraftNone);
        }

        //! Get the root schema.
        const SchemaType &GetRoot() const { return *root_; }

        //! Gets the error object.
        GValue &GetError() { return error_; }

        const GValue &GetError() const { return error_; }

        static const StringRefType &GetSchemaErrorKeyword(SchemaErrorCode schemaErrorCode) {
            switch (schemaErrorCode) {
                case kSchemaErrorStartUnknown:
                    return GetStartUnknownString();
                case kSchemaErrorRefPlainName:
                    return GetRefPlainNameString();
                case kSchemaErrorRefInvalid:
                    return GetRefInvalidString();
                case kSchemaErrorRefPointerInvalid:
                    return GetRefPointerInvalidString();
                case kSchemaErrorRefUnknown:
                    return GetRefUnknownString();
                case kSchemaErrorRefCyclical:
                    return GetRefCyclicalString();
                case kSchemaErrorRefNoRemoteProvider:
                    return GetRefNoRemoteProviderString();
                case kSchemaErrorRefNoRemoteSchema:
                    return GetRefNoRemoteSchemaString();
                case kSchemaErrorRegexInvalid:
                    return GetRegexInvalidString();
                case kSchemaErrorSpecUnknown:
                    return GetSpecUnknownString();
                case kSchemaErrorSpecUnsupported:
                    return GetSpecUnsupportedString();
                case kSchemaErrorSpecIllegal:
                    return GetSpecIllegalString();
                case kSchemaErrorReadOnlyAndWriteOnly:
                    return GetReadOnlyAndWriteOnlyString();
                default:
                    return GetNullString();
            }
        }

        //! Default error method
        void SchemaError(const SchemaErrorCode code, const PointerType &location) {
            currentError_ = GValue(kObjectType);
            AddCurrentError(code, location);
        }

        //! Method for error with single string value insert
        void
        SchemaErrorValue(const SchemaErrorCode code, const PointerType &location, const char_type *value, SizeType length) {
            currentError_ = GValue(kObjectType);
            currentError_.add_member(GetValueString(), GValue(value, length, *allocator_).Move(), *allocator_);
            AddCurrentError(code, location);
        }

        //! Method for error with invalid pointer
        void
        SchemaErrorPointer(const SchemaErrorCode code, const PointerType &location, const char_type *value, SizeType length,
                           const PointerType &pointer) {
            currentError_ = GValue(kObjectType);
            currentError_.add_member(GetValueString(), GValue(value, length, *allocator_).Move(), *allocator_);
            currentError_.add_member(GetOffsetString(),
                                    static_cast<SizeType>(pointer.GetParseErrorOffset() / sizeof(char_type)), *allocator_);
            AddCurrentError(code, location);
        }

    private:
        //! Prohibit copying
        GenericSchemaDocument(const GenericSchemaDocument &);

        //! Prohibit assignment
        GenericSchemaDocument &operator=(const GenericSchemaDocument &);

        typedef const PointerType *SchemaRefPtr; // PR #1393

        struct SchemaEntry {
            SchemaEntry(const PointerType &p, SchemaType *s, bool o, Allocator *allocator) : pointer(p, allocator),
                                                                                             schema(s), owned(o) {}

            ~SchemaEntry() {
                if (owned) {
                    schema->~SchemaType();
                    Allocator::Free(schema);
                }
            }

            PointerType pointer;
            SchemaType *schema;
            bool owned;
        };

        void AddErrorInstanceLocation(GValue &result, const PointerType &location) {
            GenericStringBuffer<EncodingType> sb;
            location.StringifyUriFragment(sb);
            GValue instanceRef(sb.get_string(), static_cast<SizeType>(sb.GetSize() / sizeof(char_type)), *allocator_);
            result.add_member(GetInstanceRefString(), instanceRef, *allocator_);
        }

        void AddError(GValue &keyword, GValue &error) {
            typename GValue::MemberIterator member = error_.find_member(keyword);
            if (member == error_.member_end())
                error_.add_member(keyword, error, *allocator_);
            else {
                if (member->value.is_object()) {
                    GValue errors(kArrayType);
                    errors.push_back(member->value, *allocator_);
                    member->value = errors;
                }
                member->value.push_back(error, *allocator_);
            }
        }

        void AddCurrentError(const SchemaErrorCode code, const PointerType &location) {
            MERAK_JSON_SCHEMA_PRINT(InvalidKeyword, GetSchemaErrorKeyword(code));
            currentError_.add_member(GetErrorCodeString(), code, *allocator_);
            AddErrorInstanceLocation(currentError_, location);
            AddError(GValue(GetSchemaErrorKeyword(code)).Move(), currentError_);
        }

#define MERAK_JSON_STRING_(name, ...) \
        static const StringRefType& Get##name##String() {\
            static const char_type s[] = { __VA_ARGS__, '\0' };\
            static const StringRefType v(s, static_cast<SizeType>(sizeof(s) / sizeof(char_type) - 1)); \
            return v;\
        }

        MERAK_JSON_STRING_(InstanceRef, 'i', 'n', 's', 't', 'a', 'n', 'c', 'e', 'R', 'e', 'f')

        MERAK_JSON_STRING_(ErrorCode, 'e', 'r', 'r', 'o', 'r', 'C', 'o', 'd', 'e')

        MERAK_JSON_STRING_(Value, 'v', 'a', 'l', 'u', 'e')

        MERAK_JSON_STRING_(Offset, 'o', 'f', 'f', 's', 'e', 't')

        MERAK_JSON_STRING_(Null, 'n', 'u', 'l', 'l')

        MERAK_JSON_STRING_(SpecUnknown, 'S', 'p', 'e', 'c', 'U', 'n', 'k', 'n', 'o', 'w', 'n')

        MERAK_JSON_STRING_(SpecUnsupported, 'S', 'p', 'e', 'c', 'U', 'n', 's', 'u', 'p', 'p', 'o', 'r', 't', 'e', 'd')

        MERAK_JSON_STRING_(SpecIllegal, 'S', 'p', 'e', 'c', 'I', 'l', 'l', 'e', 'g', 'a', 'l')

        MERAK_JSON_STRING_(StartUnknown, 'S', 't', 'a', 'r', 't', 'U', 'n', 'k', 'n', 'o', 'w', 'n')

        MERAK_JSON_STRING_(RefPlainName, 'R', 'e', 'f', 'P', 'l', 'a', 'i', 'n', 'N', 'a', 'm', 'e')

        MERAK_JSON_STRING_(RefInvalid, 'R', 'e', 'f', 'I', 'n', 'v', 'a', 'l', 'i', 'd')

        MERAK_JSON_STRING_(RefPointerInvalid, 'R', 'e', 'f', 'P', 'o', 'i', 'n', 't', 'e', 'r', 'I', 'n', 'v', 'a', 'l',
                           'i', 'd')

        MERAK_JSON_STRING_(RefUnknown, 'R', 'e', 'f', 'U', 'n', 'k', 'n', 'o', 'w', 'n')

        MERAK_JSON_STRING_(RefCyclical, 'R', 'e', 'f', 'C', 'y', 'c', 'l', 'i', 'c', 'a', 'l')

        MERAK_JSON_STRING_(RefNoRemoteProvider, 'R', 'e', 'f', 'N', 'o', 'R', 'e', 'm', 'o', 't', 'e', 'P', 'r', 'o',
                           'v', 'i', 'd', 'e', 'r')

        MERAK_JSON_STRING_(RefNoRemoteSchema, 'R', 'e', 'f', 'N', 'o', 'R', 'e', 'm', 'o', 't', 'e', 'S', 'c', 'h', 'e',
                           'm', 'a')

        MERAK_JSON_STRING_(ReadOnlyAndWriteOnly, 'R', 'e', 'a', 'd', 'O', 'n', 'l', 'y', 'A', 'n', 'd', 'W', 'r', 'i',
                           't', 'e', 'O', 'n', 'l', 'y')

        MERAK_JSON_STRING_(RegexInvalid, 'R', 'e', 'g', 'e', 'x', 'I', 'n', 'v', 'a', 'l', 'i', 'd')

#undef MERAK_JSON_STRING_

        // Static method to get schema draft of any schema document
        static SchemaDraft GetSchemaDraft(const ValueType &document) {
            static const char_type kDraft03String[] = {'h', 't', 't', 'p', ':', '/', '/', 'j', 's', 'o', 'n', '-', 's', 'c',
                                                'h', 'e', 'm', 'a', '.', 'o', 'r', 'g', '/', 'd', 'r', 'a', 'f', 't',
                                                '-', '0', '3', '/', 's', 'c', 'h', 'e', 'm', 'a', '#', '\0'};
            static const char_type kDraft04String[] = {'h', 't', 't', 'p', ':', '/', '/', 'j', 's', 'o', 'n', '-', 's', 'c',
                                                'h', 'e', 'm', 'a', '.', 'o', 'r', 'g', '/', 'd', 'r', 'a', 'f', 't',
                                                '-', '0', '4', '/', 's', 'c', 'h', 'e', 'm', 'a', '#', '\0'};
            static const char_type kDraft05String[] = {'h', 't', 't', 'p', ':', '/', '/', 'j', 's', 'o', 'n', '-', 's', 'c',
                                                'h', 'e', 'm', 'a', '.', 'o', 'r', 'g', '/', 'd', 'r', 'a', 'f', 't',
                                                '-', '0', '5', '/', 's', 'c', 'h', 'e', 'm', 'a', '#', '\0'};
            static const char_type kDraft06String[] = {'h', 't', 't', 'p', ':', '/', '/', 'j', 's', 'o', 'n', '-', 's', 'c',
                                                'h', 'e', 'm', 'a', '.', 'o', 'r', 'g', '/', 'd', 'r', 'a', 'f', 't',
                                                '-', '0', '6', '/', 's', 'c', 'h', 'e', 'm', 'a', '#', '\0'};
            static const char_type kDraft07String[] = {'h', 't', 't', 'p', ':', '/', '/', 'j', 's', 'o', 'n', '-', 's', 'c',
                                                'h', 'e', 'm', 'a', '.', 'o', 'r', 'g', '/', 'd', 'r', 'a', 'f', 't',
                                                '-', '0', '7', '/', 's', 'c', 'h', 'e', 'm', 'a', '#', '\0'};
            static const char_type kDraft2019_09String[] = {'h', 't', 't', 'p', 's', ':', '/', '/', 'j', 's', 'o', 'n', '-',
                                                     's', 'c', 'h', 'e', 'm', 'a', '.', 'o', 'r', 'g', '/', 'd', 'r',
                                                     'a', 'f', 't', '/', '2', '0', '1', '9', '-', '0', '9', '/', 's',
                                                     'c', 'h', 'e', 'm', 'a', '\0'};
            static const char_type kDraft2020_12String[] = {'h', 't', 't', 'p', 's', ':', '/', '/', 'j', 's', 'o', 'n', '-',
                                                     's', 'c', 'h', 'e', 'm', 'a', '.', 'o', 'r', 'g', '/', 'd', 'r',
                                                     'a', 'f', 't', '/', '2', '0', '2', '0', '-', '1', '2', '/', 's',
                                                     'c', 'h', 'e', 'm', 'a', '\0'};

            if (!document.is_object()) {
                return kDraftNone;
            }

            // Get the schema draft from the $schema keyword at the supplied location
            typename ValueType::ConstMemberIterator itr = document.find_member(SchemaType::GetSchemaString());
            if (itr != document.member_end()) {
                if (!itr->value.is_string()) return kDraftUnknown;
                const UriType draftUri(itr->value);
                // Check base uri for match
                if (draftUri.Match(UriType(kDraft04String), false)) return kDraft04;
                if (draftUri.Match(UriType(kDraft05String), false)) return kDraft05;
                if (draftUri.Match(UriType(kDraft06String), false)) return kDraft06;
                if (draftUri.Match(UriType(kDraft07String), false)) return kDraft07;
                if (draftUri.Match(UriType(kDraft03String), false)) return kDraft03;
                if (draftUri.Match(UriType(kDraft2019_09String), false)) return kDraft2019_09;
                if (draftUri.Match(UriType(kDraft2020_12String), false)) return kDraft2020_12;
                return kDraftUnknown;
            }
            // $schema not found
            return kDraftNone;
        }


        // Get open api version of any schema document
        static OpenApiVersion GetOpenApiVersion(const ValueType &document) {
            static const char_type kVersion20String[] = {'2', '.', '0', '\0'};
            static const char_type kVersion30String[] = {'3', '.', '0', '.', '\0'}; // ignore patch level
            static const char_type kVersion31String[] = {'3', '.', '1', '.', '\0'}; // ignore patch level
            static SizeType len = internal::StrLen<char_type>(kVersion30String);

            if (!document.is_object()) {
                return kVersionNone;
            }

            // Get the open api version from the swagger / openapi keyword at the supplied location
            typename ValueType::ConstMemberIterator itr = document.find_member(SchemaType::GetSwaggerString());
            if (itr == document.member_end()) itr = document.find_member(SchemaType::GetOpenApiString());
            if (itr != document.member_end()) {
                if (!itr->value.is_string()) return kVersionUnknown;
                const ValueType kVersion20Value(kVersion20String);
                if (kVersion20Value == itr->value) return kVersion20; // must match 2.0 exactly
                const ValueType kVersion30Value(kVersion30String);
                if (itr->value.get_string_length() > len &&
                    kVersion30Value == ValueType(itr->value.get_string(), len))
                    return kVersion30; // must match 3.0.x
                const ValueType kVersion31Value(kVersion31String);
                if (itr->value.get_string_length() > len &&
                    kVersion31Value == ValueType(itr->value.get_string(), len))
                    return kVersion31; // must match 3.1.x
                return kVersionUnknown;
            }
            // swagger or openapi not found
            return kVersionNone;
        }

        // Get the draft of the schema or the open api version (which implies the draft).
        // Report an error if schema draft or open api version not supported or not recognized, or both in document, and carry on.
        void SetSchemaSpecification(const ValueType &document) {
            // Look for '$schema', 'swagger' or 'openapi' keyword at document root
            SchemaDraft docDraft = GetSchemaDraft(document);
            OpenApiVersion docOapi = GetOpenApiVersion(document);
            // Error if both in document
            if (docDraft != kDraftNone && docOapi != kVersionNone)
                SchemaError(kSchemaErrorSpecIllegal, PointerType());
            // Use document draft or open api version if present or use spec from constructor
            if (docDraft != kDraftNone)
                spec_ = Specification(docDraft);
            else if (docOapi != kVersionNone)
                spec_ = Specification(docOapi);
            // Error if draft or version unknown
            if (spec_.draft == kDraftUnknown || spec_.oapi == kVersionUnknown)
                SchemaError(kSchemaErrorSpecUnknown, PointerType());
            else if (!spec_.IsSupported())
                SchemaError(kSchemaErrorSpecUnsupported, PointerType());
        }

        // Changed by PR #1393
        void CreateSchemaRecursive(const SchemaType **schema, const PointerType &pointer, const ValueType &v,
                                   const ValueType &document, const UriType &id) {
            if (v.get_type() == kObjectType) {
                UriType newid = UriType(CreateSchema(schema, pointer, v, document, id), allocator_);

                for (typename ValueType::ConstMemberIterator itr = v.member_begin(); itr != v.member_end(); ++itr)
                    CreateSchemaRecursive(0, pointer.Append(itr->name, allocator_), itr->value, document, newid);
            } else if (v.get_type() == kArrayType)
                for (SizeType i = 0; i < v.size(); i++)
                    CreateSchemaRecursive(0, pointer.Append(i, allocator_), v[i], document, id);
        }

        // Changed by PR #1393
        const UriType &CreateSchema(const SchemaType **schema, const PointerType &pointer, const ValueType &v,
                                    const ValueType &document, const UriType &id) {
            MERAK_JSON_ASSERT(pointer.IsValid());
            GenericStringBuffer<EncodingType> sb;
            pointer.StringifyUriFragment(sb);
            MERAK_JSON_SCHEMA_PRINT(Method, "GenericSchemaDocument::CreateSchema", sb.get_string(), id.get_string());
            if (v.is_object()) {
                if (const SchemaType *sc = GetSchema(pointer)) {
                    if (schema)
                        *schema = sc;
                    AddSchemaRefs(const_cast<SchemaType *>(sc));
                } else if (!HandleRefSchema(pointer, schema, v, document, id)) {
                    // The new schema constructor adds itself and its $ref(s) to schemaMap_
                    SchemaType *s = new(allocator_->Malloc(sizeof(SchemaType))) SchemaType(this, pointer, v, document,
                                                                                           allocator_, id);
                    if (schema)
                        *schema = s;
                    return s->GetId();
                }
            } else {
                if (schema)
                    *schema = typeless_;
                AddSchemaRefs(typeless_);
            }
            return id;
        }

        // Changed by PR #1393
        // TODO should this return a UriType& ?
        bool HandleRefSchema(const PointerType &source, const SchemaType **schema, const ValueType &v,
                             const ValueType &document, const UriType &id) {
            typename ValueType::ConstMemberIterator itr = v.find_member(SchemaType::GetRefString());
            if (itr == v.member_end())
                return false;

            GenericStringBuffer<EncodingType> sb;
            source.StringifyUriFragment(sb);
            MERAK_JSON_SCHEMA_PRINT(Method, "GenericSchemaDocument::HandleRefSchema", sb.get_string(), id.get_string());
            // Resolve the source pointer to the $ref'ed schema (finally)
            new(schemaRef_.template Push<SchemaRefPtr>()) SchemaRefPtr(&source);

            if (itr->value.is_string()) {
                SizeType len = itr->value.get_string_length();
                if (len == 0)
                    SchemaError(kSchemaErrorRefInvalid, source);
                else {
                    // First resolve $ref against the in-scope id
                    UriType scopeId = UriType(id, allocator_);
                    UriType ref = UriType(itr->value, allocator_).Resolve(scopeId, allocator_);
                    MERAK_JSON_SCHEMA_PRINT(SchemaIds, id.get_string(), itr->value.get_string(), ref.get_string());
                    // See if the resolved $ref minus the fragment matches a resolved id in this document
                    // Search from the root. Returns the subschema in the document and its absolute JSON pointer.
                    PointerType basePointer = PointerType();
                    const ValueType *base = FindId(document, ref, basePointer, docId_, false);
                    if (!base) {
                        // Remote reference - call the remote document provider
                        if (!remoteProvider_)
                            SchemaError(kSchemaErrorRefNoRemoteProvider, source);
                        else {
                            if (const GenericSchemaDocument *remoteDocument = remoteProvider_->GetRemoteDocument(ref,
                                                                                                                 spec_)) {
                                const char_type *s = ref.GetFragString();
                                len = ref.GetFragStringLength();
                                if (len <= 1 || s[1] == '/') {
                                    // JSON pointer fragment, absolute in the remote schema
                                    const PointerType pointer(s, len, allocator_);
                                    if (!pointer.IsValid())
                                        SchemaErrorPointer(kSchemaErrorRefPointerInvalid, source, s, len, pointer);
                                    else {
                                        // Get the subschema
                                        if (const SchemaType *sc = remoteDocument->GetSchema(pointer)) {
                                            if (schema)
                                                *schema = sc;
                                            AddSchemaRefs(const_cast<SchemaType *>(sc));
                                            return true;
                                        } else
                                            SchemaErrorValue(kSchemaErrorRefUnknown, source, ref.get_string(),
                                                             ref.get_string_length());
                                    }
                                } else
                                    // Plain name fragment, not allowed in remote schema
                                    SchemaErrorValue(kSchemaErrorRefPlainName, source, s, len);
                            } else
                                SchemaErrorValue(kSchemaErrorRefNoRemoteSchema, source, ref.get_string(),
                                                 ref.get_string_length());
                        }
                    } else { // Local reference
                        const char_type *s = ref.GetFragString();
                        len = ref.GetFragStringLength();
                        if (len <= 1 || s[1] == '/') {
                            // JSON pointer fragment, relative to the resolved URI
                            const PointerType relPointer(s, len, allocator_);
                            if (!relPointer.IsValid())
                                SchemaErrorPointer(kSchemaErrorRefPointerInvalid, source, s, len, relPointer);
                            else {
                                // Get the subschema
                                if (const ValueType *pv = relPointer.get(*base)) {
                                    // Now get the absolute JSON pointer by adding relative to base
                                    PointerType pointer(basePointer, allocator_);
                                    for (SizeType i = 0; i < relPointer.GetTokenCount(); i++)
                                        pointer = pointer.Append(relPointer.GetTokens()[i], allocator_);
                                    if (IsCyclicRef(pointer))
                                        SchemaErrorValue(kSchemaErrorRefCyclical, source, ref.get_string(),
                                                         ref.get_string_length());
                                    else {
                                        // Call CreateSchema recursively, but first compute the in-scope id for the $ref target as we have jumped there
                                        // TODO: cache pointer <-> id mapping
                                        size_t unresolvedTokenIndex;
                                        scopeId = pointer.GetUri(document, docId_, &unresolvedTokenIndex, allocator_);
                                        CreateSchema(schema, pointer, *pv, document, scopeId);
                                        return true;
                                    }
                                } else
                                    SchemaErrorValue(kSchemaErrorRefUnknown, source, ref.get_string(),
                                                     ref.get_string_length());
                            }
                        } else {
                            // Plain name fragment, relative to the resolved URI
                            // Not supported in open api 2.0 and 3.0
                            PointerType pointer(allocator_);
                            if (spec_.oapi == kVersion20 || spec_.oapi == kVersion30)
                                SchemaErrorValue(kSchemaErrorRefPlainName, source, s, len);
                                // See if the fragment matches an id in this document.
                                // Search from the base we just established. Returns the subschema in the document and its absolute JSON pointer.
                            else if (const ValueType *pv = FindId(*base, ref, pointer, UriType(ref.GetBaseString(),
                                                                                               ref.GetBaseStringLength(),
                                                                                               allocator_), true,
                                                                  basePointer)) {
                                if (IsCyclicRef(pointer))
                                    SchemaErrorValue(kSchemaErrorRefCyclical, source, ref.get_string(),
                                                     ref.get_string_length());
                                else {
                                    // Call CreateSchema recursively, but first compute the in-scope id for the $ref target as we have jumped there
                                    // TODO: cache pointer <-> id mapping
                                    size_t unresolvedTokenIndex;
                                    scopeId = pointer.GetUri(document, docId_, &unresolvedTokenIndex, allocator_);
                                    CreateSchema(schema, pointer, *pv, document, scopeId);
                                    return true;
                                }
                            } else
                                SchemaErrorValue(kSchemaErrorRefUnknown, source, ref.get_string(),
                                                 ref.get_string_length());
                        }
                    }
                }
            }

            // Invalid/Unknown $ref
            if (schema)
                *schema = typeless_;
            AddSchemaRefs(typeless_);
            return true;
        }

        //! Find the first subschema with a resolved 'id' that matches the specified URI.
        // If full specified use all URI else ignore fragment.
        // If found, return a pointer to the subschema and its JSON pointer.
        // TODO cache pointer <-> id mapping
        ValueType *
        FindId(const ValueType &doc, const UriType &finduri, PointerType &resptr, const UriType &baseuri, bool full,
               const PointerType &here = PointerType()) const {
            SizeType i = 0;
            ValueType *resval = 0;
            UriType tempuri = UriType(finduri, allocator_);
            UriType localuri = UriType(baseuri, allocator_);
            if (doc.get_type() == kObjectType) {
                // Establish the base URI of this object
                typename ValueType::ConstMemberIterator m = doc.find_member(SchemaType::GetIdString());
                if (m != doc.member_end() && m->value.get_type() == kStringType) {
                    localuri = UriType(m->value, allocator_).Resolve(baseuri, allocator_);
                }
                // See if it matches
                if (localuri.Match(finduri, full)) {
                    MERAK_JSON_SCHEMA_PRINT(Method, "GenericSchemaDocument::FindId (match)",
                                            full ? localuri.get_string() : localuri.GetBaseString());
                    resval = const_cast<ValueType *>(&doc);
                    resptr = here;
                    return resval;
                }
                // No match, continue looking
                for (m = doc.member_begin(); m != doc.member_end(); ++m) {
                    if (m->value.get_type() == kObjectType || m->value.get_type() == kArrayType) {
                        resval = FindId(m->value, finduri, resptr, localuri, full,
                                        here.Append(m->name.get_string(), m->name.get_string_length(), allocator_));
                    }
                    if (resval) break;
                }
            } else if (doc.get_type() == kArrayType) {
                // Continue looking
                for (typename ValueType::ConstValueIterator v = doc.begin(); v != doc.end(); ++v) {
                    if (v->get_type() == kObjectType || v->get_type() == kArrayType) {
                        resval = FindId(*v, finduri, resptr, localuri, full, here.Append(i, allocator_));
                    }
                    if (resval) break;
                    i++;
                }
            }
            return resval;
        }

        // Added by PR #1393
        void AddSchemaRefs(SchemaType *schema) {
            MERAK_JSON_SCHEMA_PRINT(Method, "GenericSchemaDocument::AddSchemaRefs");
            while (!schemaRef_.empty()) {
                SchemaRefPtr *ref = schemaRef_.template Pop<SchemaRefPtr>(1);
                SchemaEntry *entry = schemaMap_.template Push<SchemaEntry>();
                new(entry) SchemaEntry(**ref, schema, false, allocator_);
            }
        }

        // Added by PR #1393
        bool IsCyclicRef(const PointerType &pointer) const {
            for (const SchemaRefPtr *ref = schemaRef_.template Bottom<SchemaRefPtr>();
                 ref != schemaRef_.template end<SchemaRefPtr>(); ++ref)
                if (pointer == **ref)
                    return true;
            return false;
        }

        const SchemaType *GetSchema(const PointerType &pointer) const {
            for (const SchemaEntry *target = schemaMap_.template Bottom<SchemaEntry>();
                 target != schemaMap_.template end<SchemaEntry>(); ++target)
                if (pointer == target->pointer)
                    return target->schema;
            return 0;
        }

        PointerType GetPointer(const SchemaType *schema) const {
            for (const SchemaEntry *target = schemaMap_.template Bottom<SchemaEntry>();
                 target != schemaMap_.template end<SchemaEntry>(); ++target)
                if (schema == target->schema)
                    return target->pointer;
            return PointerType();
        }

        const SchemaType *GetTypeless() const { return typeless_; }

        static const size_t kInitialSchemaMapSize = 64;
        static const size_t kInitialSchemaRefSize = 64;

        IRemoteSchemaDocumentProviderType *remoteProvider_;
        Allocator *allocator_;
        Allocator *ownAllocator_;
        const SchemaType *root_;                //!< Root schema.
        SchemaType *typeless_;
        internal::Stack<Allocator> schemaMap_;  // Stores created Pointer -> Schemas
        internal::Stack<Allocator> schemaRef_;  // Stores Pointer(s) from $ref(s) until resolved
        GValue uri_;                            // Schema document URI
        UriType docId_;
        Specification spec_;
        GValue error_;
        GValue currentError_;
    };

    //! GenericSchemaDocument using Value type.
    typedef GenericSchemaDocument<Value> SchemaDocument;
    //! IGenericRemoteSchemaDocumentProvider using SchemaDocument.
    typedef IGenericRemoteSchemaDocumentProvider<SchemaDocument> IRemoteSchemaDocumentProvider;

    ///////////////////////////////////////////////////////////////////////////////
    // GenericSchemaValidator

    //! JSON Schema Validator.
    /*!
        A SAX style JSON schema validator.
        It uses a \c GenericSchemaDocument to validate SAX events.
        It delegates the incoming SAX events to an output handler.
        The default output handler does nothing.
        It can be reused multiple times by calling \c Reset().

        \tparam SchemaDocumentType Type of schema document.
        \tparam OutputHandler Type of output handler. Default handler does nothing.
        \tparam StateAllocator Allocator for storing the internal validation states.
    */
    template<
            typename SchemaDocumentType,
            typename OutputHandler = BaseReaderHandler<typename SchemaDocumentType::SchemaType::EncodingType>,
            typename StateAllocator = CrtAllocator>
    class GenericSchemaValidator :
            public internal::ISchemaStateFactory<typename SchemaDocumentType::SchemaType>,
            public internal::ISchemaValidator,
            public internal::IValidationErrorHandler<typename SchemaDocumentType::SchemaType> {
    public:
        typedef typename SchemaDocumentType::SchemaType SchemaType;
        typedef typename SchemaDocumentType::PointerType PointerType;
        typedef typename SchemaType::EncodingType EncodingType;
        typedef typename SchemaType::SValue SValue;
        typedef typename EncodingType::char_type char_type;
        typedef GenericStringRef<char_type> StringRefType;
        typedef GenericValue<EncodingType, StateAllocator> ValueType;

        //! Constructor without output handler.
        /*!
            \param schemaDocument The schema document to conform to.
            \param allocator Optional allocator for storing internal validation states.
            \param schemaStackCapacity Optional initial capacity of schema path stack.
            \param documentStackCapacity Optional initial capacity of document path stack.
        */
        GenericSchemaValidator(
                const SchemaDocumentType &schemaDocument,
                StateAllocator *allocator = 0,
                size_t schemaStackCapacity = kDefaultSchemaStackCapacity,
                size_t documentStackCapacity = kDefaultDocumentStackCapacity)
                :
                schemaDocument_(&schemaDocument),
                root_(schemaDocument.GetRoot()),
                stateAllocator_(allocator),
                ownStateAllocator_(0),
                schemaStack_(allocator, schemaStackCapacity),
                documentStack_(allocator, documentStackCapacity),
                outputHandler_(0),
                error_(kObjectType),
                currentError_(),
                missingDependents_(),
                valid_(true),
                flags_(kValidateDefaultFlags),
                depth_(0) {
            MERAK_JSON_SCHEMA_PRINT(Method, "GenericSchemaValidator::GenericSchemaValidator");
        }

        //! Constructor with output handler.
        /*!
            \param schemaDocument The schema document to conform to.
            \param allocator Optional allocator for storing internal validation states.
            \param schemaStackCapacity Optional initial capacity of schema path stack.
            \param documentStackCapacity Optional initial capacity of document path stack.
        */
        GenericSchemaValidator(
                const SchemaDocumentType &schemaDocument,
                OutputHandler &outputHandler,
                StateAllocator *allocator = 0,
                size_t schemaStackCapacity = kDefaultSchemaStackCapacity,
                size_t documentStackCapacity = kDefaultDocumentStackCapacity)
                :
                schemaDocument_(&schemaDocument),
                root_(schemaDocument.GetRoot()),
                stateAllocator_(allocator),
                ownStateAllocator_(0),
                schemaStack_(allocator, schemaStackCapacity),
                documentStack_(allocator, documentStackCapacity),
                outputHandler_(&outputHandler),
                error_(kObjectType),
                currentError_(),
                missingDependents_(),
                valid_(true),
                flags_(kValidateDefaultFlags),
                depth_(0) {
            MERAK_JSON_SCHEMA_PRINT(Method, "GenericSchemaValidator::GenericSchemaValidator (output handler)");
        }

        //! Destructor.
        ~GenericSchemaValidator() {
            Reset();
            MERAK_JSON_DELETE(ownStateAllocator_);
        }

        //! Reset the internal states.
        void Reset() {
            while (!schemaStack_.empty())
                PopSchema();
            documentStack_.clear();
            ResetError();
        }

        //! Reset the error state.
        void ResetError() {
            error_.SetObject();
            currentError_.SetNull();
            missingDependents_.SetNull();
            valid_ = true;
        }

        //! Implementation of ISchemaValidator
        void SetValidateFlags(unsigned flags) {
            flags_ = flags;
        }

        virtual unsigned GetValidateFlags() const {
            return flags_;
        }

        virtual bool IsValid() const {
            if (!valid_) return false;
            if (GetContinueOnErrors() && !error_.object_empty()) return false;
            return true;
        }
        //! end of Implementation of ISchemaValidator

        //! Gets the error object.
        ValueType &GetError() { return error_; }

        const ValueType &GetError() const { return error_; }

        //! Gets the JSON pointer pointed to the invalid schema.
        //  If reporting all errors, the stack will be empty.
        PointerType GetInvalidSchemaPointer() const {
            return schemaStack_.empty() ? PointerType() : CurrentSchema().GetPointer();
        }

        //! Gets the keyword of invalid schema.
        //  If reporting all errors, the stack will be empty, so return "errors".
        const char_type *GetInvalidSchemaKeyword() const {
            if (!schemaStack_.empty()) return CurrentContext().invalidKeyword;
            if (GetContinueOnErrors() && !error_.object_empty()) return static_cast<const char_type *>(GetErrorsString());
            return 0;
        }

        //! Gets the error code of invalid schema.
        //  If reporting all errors, the stack will be empty, so return kValidateErrors.
        ValidateErrorCode GetInvalidSchemaCode() const {
            if (!schemaStack_.empty()) return CurrentContext().invalidCode;
            if (GetContinueOnErrors() && !error_.object_empty()) return kValidateErrors;
            return kValidateErrorNone;
        }

        //! Gets the JSON pointer pointed to the invalid value.
        //  If reporting all errors, the stack will be empty.
        PointerType GetInvalidDocumentPointer() const {
            if (documentStack_.empty()) {
                return PointerType();
            } else {
                return PointerType(documentStack_.template Bottom<char_type>(), documentStack_.GetSize() / sizeof(char_type));
            }
        }

        void NotMultipleOf(int64_t actual, const SValue &expected) {
            AddNumberError(kValidateErrorMultipleOf, ValueType(actual).Move(), expected);
        }

        void NotMultipleOf(uint64_t actual, const SValue &expected) {
            AddNumberError(kValidateErrorMultipleOf, ValueType(actual).Move(), expected);
        }

        void NotMultipleOf(double actual, const SValue &expected) {
            AddNumberError(kValidateErrorMultipleOf, ValueType(actual).Move(), expected);
        }

        void AboveMaximum(int64_t actual, const SValue &expected, bool exclusive) {
            AddNumberError(exclusive ? kValidateErrorExclusiveMaximum : kValidateErrorMaximum, ValueType(actual).Move(),
                           expected,
                           exclusive ? &SchemaType::GetExclusiveMaximumString : 0);
        }

        void AboveMaximum(uint64_t actual, const SValue &expected, bool exclusive) {
            AddNumberError(exclusive ? kValidateErrorExclusiveMaximum : kValidateErrorMaximum, ValueType(actual).Move(),
                           expected,
                           exclusive ? &SchemaType::GetExclusiveMaximumString : 0);
        }

        void AboveMaximum(double actual, const SValue &expected, bool exclusive) {
            AddNumberError(exclusive ? kValidateErrorExclusiveMaximum : kValidateErrorMaximum, ValueType(actual).Move(),
                           expected,
                           exclusive ? &SchemaType::GetExclusiveMaximumString : 0);
        }

        void BelowMinimum(int64_t actual, const SValue &expected, bool exclusive) {
            AddNumberError(exclusive ? kValidateErrorExclusiveMinimum : kValidateErrorMinimum, ValueType(actual).Move(),
                           expected,
                           exclusive ? &SchemaType::GetExclusiveMinimumString : 0);
        }

        void BelowMinimum(uint64_t actual, const SValue &expected, bool exclusive) {
            AddNumberError(exclusive ? kValidateErrorExclusiveMinimum : kValidateErrorMinimum, ValueType(actual).Move(),
                           expected,
                           exclusive ? &SchemaType::GetExclusiveMinimumString : 0);
        }

        void BelowMinimum(double actual, const SValue &expected, bool exclusive) {
            AddNumberError(exclusive ? kValidateErrorExclusiveMinimum : kValidateErrorMinimum, ValueType(actual).Move(),
                           expected,
                           exclusive ? &SchemaType::GetExclusiveMinimumString : 0);
        }

        void TooLong(const char_type *str, SizeType length, SizeType expected) {
            AddNumberError(kValidateErrorMaxLength,
                           ValueType(str, length, GetStateAllocator()).Move(), SValue(expected).Move());
        }

        void TooShort(const char_type *str, SizeType length, SizeType expected) {
            AddNumberError(kValidateErrorMinLength,
                           ValueType(str, length, GetStateAllocator()).Move(), SValue(expected).Move());
        }

        void DoesNotMatch(const char_type *str, SizeType length) {
            currentError_.SetObject();
            currentError_.add_member(GetActualString(), ValueType(str, length, GetStateAllocator()).Move(),
                                    GetStateAllocator());
            AddCurrentError(kValidateErrorPattern);
        }

        void DisallowedItem(SizeType index) {
            currentError_.SetObject();
            currentError_.add_member(GetDisallowedString(), ValueType(index).Move(), GetStateAllocator());
            AddCurrentError(kValidateErrorAdditionalItems, true);
        }

        void TooFewItems(SizeType actualCount, SizeType expectedCount) {
            AddNumberError(kValidateErrorMinItems,
                           ValueType(actualCount).Move(), SValue(expectedCount).Move());
        }

        void TooManyItems(SizeType actualCount, SizeType expectedCount) {
            AddNumberError(kValidateErrorMaxItems,
                           ValueType(actualCount).Move(), SValue(expectedCount).Move());
        }

        void DuplicateItems(SizeType index1, SizeType index2) {
            ValueType duplicates(kArrayType);
            duplicates.push_back(index1, GetStateAllocator());
            duplicates.push_back(index2, GetStateAllocator());
            currentError_.SetObject();
            currentError_.add_member(GetDuplicatesString(), duplicates, GetStateAllocator());
            AddCurrentError(kValidateErrorUniqueItems, true);
        }

        void TooManyProperties(SizeType actualCount, SizeType expectedCount) {
            AddNumberError(kValidateErrorMaxProperties,
                           ValueType(actualCount).Move(), SValue(expectedCount).Move());
        }

        void TooFewProperties(SizeType actualCount, SizeType expectedCount) {
            AddNumberError(kValidateErrorMinProperties,
                           ValueType(actualCount).Move(), SValue(expectedCount).Move());
        }

        void StartMissingProperties() {
            currentError_.set_array();
        }

        void AddMissingProperty(const SValue &name) {
            currentError_.push_back(ValueType(name, GetStateAllocator()).Move(), GetStateAllocator());
        }

        bool EndMissingProperties() {
            if (currentError_.empty())
                return false;
            ValueType error(kObjectType);
            error.add_member(GetMissingString(), currentError_, GetStateAllocator());
            currentError_ = error;
            AddCurrentError(kValidateErrorRequired);
            return true;
        }

        void PropertyViolations(ISchemaValidator **subvalidators, SizeType count) {
            for (SizeType i = 0; i < count; ++i)
                MergeError(static_cast<GenericSchemaValidator *>(subvalidators[i])->GetError());
        }

        void DisallowedProperty(const char_type *name, SizeType length) {
            currentError_.SetObject();
            currentError_.add_member(GetDisallowedString(), ValueType(name, length, GetStateAllocator()).Move(),
                                    GetStateAllocator());
            AddCurrentError(kValidateErrorAdditionalProperties, true);
        }

        void StartDependencyErrors() {
            currentError_.SetObject();
        }

        void StartMissingDependentProperties() {
            missingDependents_.set_array();
        }

        void AddMissingDependentProperty(const SValue &targetName) {
            missingDependents_.push_back(ValueType(targetName, GetStateAllocator()).Move(), GetStateAllocator());
        }

        void EndMissingDependentProperties(const SValue &sourceName) {
            if (!missingDependents_.empty()) {
                // Create equivalent 'required' error
                ValueType error(kObjectType);
                ValidateErrorCode code = kValidateErrorRequired;
                error.add_member(GetMissingString(), missingDependents_.Move(), GetStateAllocator());
                AddErrorCode(error, code);
                AddErrorInstanceLocation(error, false);
                // When appending to a pointer ensure its allocator is used
                PointerType schemaRef = GetInvalidSchemaPointer().Append(
                        SchemaType::GetValidateErrorKeyword(kValidateErrorDependencies),
                        &GetInvalidSchemaPointer().get_allocator());
                AddErrorSchemaLocation(error, schemaRef.Append(sourceName.get_string(), sourceName.get_string_length(),
                                                               &GetInvalidSchemaPointer().get_allocator()));
                ValueType wrapper(kObjectType);
                wrapper.add_member(ValueType(SchemaType::GetValidateErrorKeyword(code), GetStateAllocator()).Move(),
                                  error, GetStateAllocator());
                currentError_.add_member(ValueType(sourceName, GetStateAllocator()).Move(), wrapper,
                                        GetStateAllocator());
            }
        }

        void AddDependencySchemaError(const SValue &sourceName, ISchemaValidator *subvalidator) {
            currentError_.add_member(ValueType(sourceName, GetStateAllocator()).Move(),
                                    static_cast<GenericSchemaValidator *>(subvalidator)->GetError(),
                                    GetStateAllocator());
        }

        bool EndDependencyErrors() {
            if (currentError_.object_empty())
                return false;
            ValueType error(kObjectType);
            error.add_member(GetErrorsString(), currentError_, GetStateAllocator());
            currentError_ = error;
            AddCurrentError(kValidateErrorDependencies);
            return true;
        }

        void DisallowedValue(const ValidateErrorCode code = kValidateErrorEnum) {
            currentError_.SetObject();
            AddCurrentError(code);
        }

        void StartDisallowedType() {
            currentError_.set_array();
        }

        void AddExpectedType(const typename SchemaType::ValueType &expectedType) {
            currentError_.push_back(ValueType(expectedType, GetStateAllocator()).Move(), GetStateAllocator());
        }

        void EndDisallowedType(const typename SchemaType::ValueType &actualType) {
            ValueType error(kObjectType);
            error.add_member(GetExpectedString(), currentError_, GetStateAllocator());
            error.add_member(GetActualString(), ValueType(actualType, GetStateAllocator()).Move(), GetStateAllocator());
            currentError_ = error;
            AddCurrentError(kValidateErrorType);
        }

        void NotAllOf(ISchemaValidator **subvalidators, SizeType count) {
            // Treat allOf like oneOf and anyOf to match https://rapidjson.org/md_doc_schema.html#allOf-anyOf-oneOf
            AddErrorArray(kValidateErrorAllOf, subvalidators, count);
            //for (SizeType i = 0; i < count; ++i) {
            //    MergeError(static_cast<GenericSchemaValidator*>(subvalidators[i])->GetError());
            //}
        }

        void NoneOf(ISchemaValidator **subvalidators, SizeType count) {
            AddErrorArray(kValidateErrorAnyOf, subvalidators, count);
        }

        void NotOneOf(ISchemaValidator **subvalidators, SizeType count) {
            AddErrorArray(kValidateErrorOneOf, subvalidators, count);
        }

        void MultipleOneOf(SizeType index1, SizeType index2) {
            ValueType matches(kArrayType);
            matches.push_back(index1, GetStateAllocator());
            matches.push_back(index2, GetStateAllocator());
            currentError_.SetObject();
            currentError_.add_member(GetMatchesString(), matches, GetStateAllocator());
            AddCurrentError(kValidateErrorOneOfMatch);
        }

        void Disallowed() {
            currentError_.SetObject();
            AddCurrentError(kValidateErrorNot);
        }

        void DisallowedWhenWriting() {
            currentError_.SetObject();
            AddCurrentError(kValidateErrorReadOnly);
        }

        void DisallowedWhenReading() {
            currentError_.SetObject();
            AddCurrentError(kValidateErrorWriteOnly);
        }

#define MERAK_JSON_STRING_(name, ...) \
        static const StringRefType& Get##name##String() {\
            static const char_type s[] = { __VA_ARGS__, '\0' };\
            static const StringRefType v(s, static_cast<SizeType>(sizeof(s) / sizeof(char_type) - 1)); \
            return v;\
        }

        MERAK_JSON_STRING_(InstanceRef, 'i', 'n', 's', 't', 'a', 'n', 'c', 'e', 'R', 'e', 'f')

        MERAK_JSON_STRING_(SchemaRef, 's', 'c', 'h', 'e', 'm', 'a', 'R', 'e', 'f')

        MERAK_JSON_STRING_(Expected, 'e', 'x', 'p', 'e', 'c', 't', 'e', 'd')

        MERAK_JSON_STRING_(Actual, 'a', 'c', 't', 'u', 'a', 'l')

        MERAK_JSON_STRING_(Disallowed, 'd', 'i', 's', 'a', 'l', 'l', 'o', 'w', 'e', 'd')

        MERAK_JSON_STRING_(Missing, 'm', 'i', 's', 's', 'i', 'n', 'g')

        MERAK_JSON_STRING_(Errors, 'e', 'r', 'r', 'o', 'r', 's')

        MERAK_JSON_STRING_(ErrorCode, 'e', 'r', 'r', 'o', 'r', 'C', 'o', 'd', 'e')

        MERAK_JSON_STRING_(ErrorMessage, 'e', 'r', 'r', 'o', 'r', 'M', 'e', 's', 's', 'a', 'g', 'e')

        MERAK_JSON_STRING_(Duplicates, 'd', 'u', 'p', 'l', 'i', 'c', 'a', 't', 'e', 's')

        MERAK_JSON_STRING_(Matches, 'm', 'a', 't', 'c', 'h', 'e', 's')

#undef MERAK_JSON_STRING_

#define MERAK_JSON_SCHEMA_HANDLE_BEGIN_(method, arg1)\
        if (!valid_) return false; \
        if ((!BeginValue() && !GetContinueOnErrors()) || (!CurrentSchema().method arg1 && !GetContinueOnErrors())) {\
            *documentStack_.template Push<char_type>() = '\0';\
            documentStack_.template Pop<char_type>(1);\
            MERAK_JSON_SCHEMA_PRINT(InvalidDocument, documentStack_.template Bottom<char_type>());\
            valid_ = false;\
            return valid_;\
        }

#define MERAK_JSON_SCHEMA_HANDLE_PARALLEL_(method, arg2)\
        for (Context* context = schemaStack_.template Bottom<Context>(); context != schemaStack_.template end<Context>(); context++) {\
            if (context->hasher)\
                static_cast<HasherType*>(context->hasher)->method arg2;\
            if (context->validators)\
                for (SizeType i_ = 0; i_ < context->validatorCount; i_++)\
                    static_cast<GenericSchemaValidator*>(context->validators[i_])->method arg2;\
            if (context->patternPropertiesValidators)\
                for (SizeType i_ = 0; i_ < context->patternPropertiesValidatorCount; i_++)\
                    static_cast<GenericSchemaValidator*>(context->patternPropertiesValidators[i_])->method arg2;\
        }

#define MERAK_JSON_SCHEMA_HANDLE_END_(method, arg2)\
        valid_ = (EndValue() || GetContinueOnErrors()) && (!outputHandler_ || outputHandler_->method arg2);\
        return valid_;

#define MERAK_JSON_SCHEMA_HANDLE_VALUE_(method, arg1, arg2) \
        MERAK_JSON_SCHEMA_HANDLE_BEGIN_   (method, arg1);\
        MERAK_JSON_SCHEMA_HANDLE_PARALLEL_(method, arg2);\
        MERAK_JSON_SCHEMA_HANDLE_END_     (method, arg2)

        bool emplace_null() { MERAK_JSON_SCHEMA_HANDLE_VALUE_(emplace_null, (CurrentContext()), ()); }

        bool emplace_bool(bool b) { MERAK_JSON_SCHEMA_HANDLE_VALUE_(emplace_bool, (CurrentContext(), b), (b)); }

        bool emplace_int32(int i) { MERAK_JSON_SCHEMA_HANDLE_VALUE_(emplace_int32, (CurrentContext(), i), (i)); }

        bool emplace_uint32(unsigned u) { MERAK_JSON_SCHEMA_HANDLE_VALUE_(emplace_uint32, (CurrentContext(), u), (u)); }

        bool emplace_int64(int64_t i) { MERAK_JSON_SCHEMA_HANDLE_VALUE_(emplace_int64, (CurrentContext(), i), (i)); }

        bool emplace_uint64(uint64_t u) { MERAK_JSON_SCHEMA_HANDLE_VALUE_(emplace_uint64, (CurrentContext(), u), (u)); }

        bool emplace_double(double d) { MERAK_JSON_SCHEMA_HANDLE_VALUE_(emplace_double, (CurrentContext(), d), (d)); }

        bool raw_number(const char_type *str, SizeType length, bool copy) {
            MERAK_JSON_SCHEMA_HANDLE_VALUE_(emplace_string, (CurrentContext(), str, length, copy), (str, length, copy));
        }

        bool emplace_string(const char_type *str, SizeType length, bool copy) {
            MERAK_JSON_SCHEMA_HANDLE_VALUE_(emplace_string, (CurrentContext(), str, length, copy), (str, length, copy));
        }

        bool start_object() {
            MERAK_JSON_SCHEMA_PRINT(Method, "GenericSchemaValidator::start_object");
            MERAK_JSON_SCHEMA_HANDLE_BEGIN_(start_object, (CurrentContext()));
            MERAK_JSON_SCHEMA_HANDLE_PARALLEL_(start_object, ());
            valid_ = !outputHandler_ || outputHandler_->start_object();
            return valid_;
        }

        bool emplace_key(const char_type *str, SizeType len, bool copy) {
            MERAK_JSON_SCHEMA_PRINT(Method, "GenericSchemaValidator::Key", str);
            if (!valid_) return false;
            AppendToken(str, len);
            if (!CurrentSchema().emplace_key(CurrentContext(), str, len, copy) && !GetContinueOnErrors()) {
                valid_ = false;
                return valid_;
            }
            MERAK_JSON_SCHEMA_HANDLE_PARALLEL_(emplace_key, (str, len, copy));
            valid_ = !outputHandler_ || outputHandler_->emplace_key(str, len, copy);
            return valid_;
        }

        bool end_object(SizeType memberCount) {
            MERAK_JSON_SCHEMA_PRINT(Method, "GenericSchemaValidator::end_object");
            if (!valid_) return false;
            MERAK_JSON_SCHEMA_HANDLE_PARALLEL_(end_object, (memberCount));
            if (!CurrentSchema().end_object(CurrentContext(), memberCount) && !GetContinueOnErrors()) {
                valid_ = false;
                return valid_;
            }
            MERAK_JSON_SCHEMA_HANDLE_END_(end_object, (memberCount));
        }

        bool start_array() {
            MERAK_JSON_SCHEMA_PRINT(Method, "GenericSchemaValidator::start_array");
            MERAK_JSON_SCHEMA_HANDLE_BEGIN_(start_array, (CurrentContext()));
            MERAK_JSON_SCHEMA_HANDLE_PARALLEL_(start_array, ());
            valid_ = !outputHandler_ || outputHandler_->start_array();
            return valid_;
        }

        bool end_array(SizeType elementCount) {
            MERAK_JSON_SCHEMA_PRINT(Method, "GenericSchemaValidator::end_array");
            if (!valid_) return false;
            MERAK_JSON_SCHEMA_HANDLE_PARALLEL_(end_array, (elementCount));
            if (!CurrentSchema().end_array(CurrentContext(), elementCount) && !GetContinueOnErrors()) {
                valid_ = false;
                return valid_;
            }
            MERAK_JSON_SCHEMA_HANDLE_END_(end_array, (elementCount));
        }

#undef MERAK_JSON_SCHEMA_HANDLE_BEGIN_
#undef MERAK_JSON_SCHEMA_HANDLE_PARALLEL_
#undef MERAK_JSON_SCHEMA_HANDLE_VALUE_

        // Implementation of ISchemaStateFactory<SchemaType>
        virtual ISchemaValidator *CreateSchemaValidator(const SchemaType &root, const bool inheritContinueOnErrors) {
            *documentStack_.template Push<char_type>() = '\0';
            documentStack_.template Pop<char_type>(1);
            ISchemaValidator *sv = new(
                    GetStateAllocator().Malloc(sizeof(GenericSchemaValidator))) GenericSchemaValidator(*schemaDocument_,
                                                                                                       root,
                                                                                                       documentStack_.template Bottom<char>(),
                                                                                                       documentStack_.GetSize(),
                                                                                                       depth_ + 1,
                                                                                                       &GetStateAllocator());
            sv->SetValidateFlags(inheritContinueOnErrors ? GetValidateFlags() : GetValidateFlags() &
                                                                                ~static_cast<unsigned>(kValidateContinueOnErrorFlag));
            return sv;
        }

        virtual void DestroySchemaValidator(ISchemaValidator *validator) {
            GenericSchemaValidator *v = static_cast<GenericSchemaValidator *>(validator);
            v->~GenericSchemaValidator();
            StateAllocator::Free(v);
        }

        virtual void *CreateHasher() {
            return new(GetStateAllocator().Malloc(sizeof(HasherType))) HasherType(&GetStateAllocator());
        }

        virtual uint64_t GetHashCode(void *hasher) {
            return static_cast<HasherType *>(hasher)->GetHashCode();
        }

        virtual void DestroryHasher(void *hasher) {
            HasherType *h = static_cast<HasherType *>(hasher);
            h->~HasherType();
            StateAllocator::Free(h);
        }

        virtual void *MallocState(size_t size) {
            return GetStateAllocator().Malloc(size);
        }

        virtual void FreeState(void *p) {
            StateAllocator::Free(p);
        }
        // end of implementation of ISchemaStateFactory<SchemaType>

    private:
        typedef typename SchemaType::Context Context;
        typedef GenericValue<UTF8<>, StateAllocator> HashCodeArray;
        typedef internal::Hasher<EncodingType, StateAllocator> HasherType;

        GenericSchemaValidator(
                const SchemaDocumentType &schemaDocument,
                const SchemaType &root,
                const char *basePath, size_t basePathSize,
                unsigned depth,
                StateAllocator *allocator = 0,
                size_t schemaStackCapacity = kDefaultSchemaStackCapacity,
                size_t documentStackCapacity = kDefaultDocumentStackCapacity)
                :
                schemaDocument_(&schemaDocument),
                root_(root),
                stateAllocator_(allocator),
                ownStateAllocator_(0),
                schemaStack_(allocator, schemaStackCapacity),
                documentStack_(allocator, documentStackCapacity),
                outputHandler_(0),
                error_(kObjectType),
                currentError_(),
                missingDependents_(),
                valid_(true),
                flags_(kValidateDefaultFlags),
                depth_(depth) {
            MERAK_JSON_SCHEMA_PRINT(Method, "GenericSchemaValidator::GenericSchemaValidator (internal)",
                                    basePath && basePathSize ? basePath : "");
            if (basePath && basePathSize)
                memcpy(documentStack_.template Push<char>(basePathSize), basePath, basePathSize);
        }

        StateAllocator &GetStateAllocator() {
            if (!stateAllocator_)
                stateAllocator_ = ownStateAllocator_ = MERAK_JSON_NEW(StateAllocator)();
            return *stateAllocator_;
        }

        bool GetContinueOnErrors() const {
            return flags_ & kValidateContinueOnErrorFlag;
        }

        bool BeginValue() {
            MERAK_JSON_SCHEMA_PRINT(Method, "GenericSchemaValidator::BeginValue");
            if (schemaStack_.empty())
                PushSchema(root_);
            else {
                if (CurrentContext().inArray)
                    internal::TokenHelper<internal::Stack<StateAllocator>, char_type>::AppendIndexToken(documentStack_,
                                                                                                 CurrentContext().arrayElementIndex);

                if (!CurrentSchema().BeginValue(CurrentContext()) && !GetContinueOnErrors())
                    return false;

                SizeType count = CurrentContext().patternPropertiesSchemaCount;
                const SchemaType **sa = CurrentContext().patternPropertiesSchemas;
                typename Context::PatternValidatorType patternValidatorType = CurrentContext().valuePatternValidatorType;
                bool valueUniqueness = CurrentContext().valueUniqueness;
                MERAK_JSON_ASSERT(CurrentContext().valueSchema);
                PushSchema(*CurrentContext().valueSchema);

                if (count > 0) {
                    CurrentContext().objectPatternValidatorType = patternValidatorType;
                    ISchemaValidator **&va = CurrentContext().patternPropertiesValidators;
                    SizeType &validatorCount = CurrentContext().patternPropertiesValidatorCount;
                    va = static_cast<ISchemaValidator **>(MallocState(sizeof(ISchemaValidator *) * count));
                    std::memset(va, 0, sizeof(ISchemaValidator *) * count);
                    for (SizeType i = 0; i < count; i++)
                        va[validatorCount++] = CreateSchemaValidator(*sa[i], true);  // Inherit continueOnError
                }

                CurrentContext().arrayUniqueness = valueUniqueness;
            }
            return true;
        }

        bool EndValue() {
            MERAK_JSON_SCHEMA_PRINT(Method, "GenericSchemaValidator::EndValue");
            if (!CurrentSchema().EndValue(CurrentContext()) && !GetContinueOnErrors())
                return false;

            GenericStringBuffer<EncodingType> sb;
            schemaDocument_->GetPointer(&CurrentSchema()).StringifyUriFragment(sb);
            *documentStack_.template Push<char_type>() = '\0';
            documentStack_.template Pop<char_type>(1);
            MERAK_JSON_SCHEMA_PRINT(ValidatorPointers, sb.get_string(), documentStack_.template Bottom<char_type>(), depth_);
            void *hasher = CurrentContext().hasher;
            uint64_t h =
                    hasher && CurrentContext().arrayUniqueness ? static_cast<HasherType *>(hasher)->GetHashCode() : 0;

            PopSchema();

            if (!schemaStack_.empty()) {
                Context &context = CurrentContext();
                // Only check uniqueness if there is a hasher
                if (hasher && context.valueUniqueness) {
                    HashCodeArray *a = static_cast<HashCodeArray *>(context.arrayElementHashCodes);
                    if (!a)
                        CurrentContext().arrayElementHashCodes = a = new(
                                GetStateAllocator().Malloc(sizeof(HashCodeArray))) HashCodeArray(kArrayType);
                    for (typename HashCodeArray::ConstValueIterator itr = a->begin(); itr != a->end(); ++itr)
                        if (itr->get_uint64() == h) {
                            DuplicateItems(static_cast<SizeType>(itr - a->begin()), a->size());
                            // Cleanup before returning if continuing
                            if (GetContinueOnErrors()) {
                                a->push_back(h, GetStateAllocator());
                                while (!documentStack_.empty() && *documentStack_.template Pop<char_type>(1) != '/');
                            }
                            MERAK_JSON_INVALID_KEYWORD_RETURN(kValidateErrorUniqueItems);
                        }
                    a->push_back(h, GetStateAllocator());
                }
            }

            // Remove the last token of document pointer
            while (!documentStack_.empty() && *documentStack_.template Pop<char_type>(1) != '/');

            return true;
        }

        void AppendToken(const char_type *str, SizeType len) {
            documentStack_.template reserve<char_type>(1 + len * 2); // worst case all characters are escaped as two characters
            *documentStack_.template PushUnsafe<char_type>() = '/';
            for (SizeType i = 0; i < len; i++) {
                if (str[i] == '~') {
                    *documentStack_.template PushUnsafe<char_type>() = '~';
                    *documentStack_.template PushUnsafe<char_type>() = '0';
                } else if (str[i] == '/') {
                    *documentStack_.template PushUnsafe<char_type>() = '~';
                    *documentStack_.template PushUnsafe<char_type>() = '1';
                } else
                    *documentStack_.template PushUnsafe<char_type>() = str[i];
            }
        }

        MERAK_JSON_FORCEINLINE void PushSchema(const SchemaType &schema) {
            new(schemaStack_.template Push<Context>()) Context(*this, *this, &schema, flags_);
        }

        MERAK_JSON_FORCEINLINE void PopSchema() {
            Context *c = schemaStack_.template Pop<Context>(1);
            if (HashCodeArray * a = static_cast<HashCodeArray *>(c->arrayElementHashCodes)) {
                a->~HashCodeArray();
                StateAllocator::Free(a);
            }
            c->~Context();
        }

        void AddErrorInstanceLocation(ValueType &result, bool parent) {
            GenericStringBuffer<EncodingType> sb;
            PointerType instancePointer = GetInvalidDocumentPointer();
            ((parent && instancePointer.GetTokenCount() > 0)
             ? PointerType(instancePointer.GetTokens(), instancePointer.GetTokenCount() - 1)
             : instancePointer).StringifyUriFragment(sb);
            ValueType instanceRef(sb.get_string(), static_cast<SizeType>(sb.GetSize() / sizeof(char_type)),
                                  GetStateAllocator());
            result.add_member(GetInstanceRefString(), instanceRef, GetStateAllocator());
        }

        void AddErrorSchemaLocation(ValueType &result, PointerType schema = PointerType()) {
            GenericStringBuffer<EncodingType> sb;
            SizeType len = CurrentSchema().GetURI().get_string_length();
            if (len) memcpy(sb.Push(len), CurrentSchema().GetURI().get_string(), len * sizeof(char_type));
            if (schema.GetTokenCount()) schema.StringifyUriFragment(sb);
            else GetInvalidSchemaPointer().StringifyUriFragment(sb);
            ValueType schemaRef(sb.get_string(), static_cast<SizeType>(sb.GetSize() / sizeof(char_type)),
                                GetStateAllocator());
            result.add_member(GetSchemaRefString(), schemaRef, GetStateAllocator());
        }

        void AddErrorCode(ValueType &result, const ValidateErrorCode code) {
            result.add_member(GetErrorCodeString(), code, GetStateAllocator());
        }

        void AddError(ValueType &keyword, ValueType &error) {
            typename ValueType::MemberIterator member = error_.find_member(keyword);
            if (member == error_.member_end())
                error_.add_member(keyword, error, GetStateAllocator());
            else {
                if (member->value.is_object()) {
                    ValueType errors(kArrayType);
                    errors.push_back(member->value, GetStateAllocator());
                    member->value = errors;
                }
                member->value.push_back(error, GetStateAllocator());
            }
        }

        void AddCurrentError(const ValidateErrorCode code, bool parent = false) {
            AddErrorCode(currentError_, code);
            AddErrorInstanceLocation(currentError_, parent);
            AddErrorSchemaLocation(currentError_);
            AddError(ValueType(SchemaType::GetValidateErrorKeyword(code), GetStateAllocator(), false).Move(),
                     currentError_);
        }

        void MergeError(ValueType &other) {
            for (typename ValueType::MemberIterator it = other.member_begin(), end = other.member_end();
                 it != end; ++it) {
                AddError(it->name, it->value);
            }
        }

        void AddNumberError(const ValidateErrorCode code, ValueType &actual, const SValue &expected,
                            const typename SchemaType::ValueType &(*exclusive)() = 0) {
            currentError_.SetObject();
            currentError_.add_member(GetActualString(), actual, GetStateAllocator());
            currentError_.add_member(GetExpectedString(), ValueType(expected, GetStateAllocator()).Move(),
                                    GetStateAllocator());
            if (exclusive)
                currentError_.add_member(ValueType(exclusive(), GetStateAllocator()).Move(), true, GetStateAllocator());
            AddCurrentError(code);
        }

        void AddErrorArray(const ValidateErrorCode code,
                           ISchemaValidator **subvalidators, SizeType count) {
            ValueType errors(kArrayType);
            for (SizeType i = 0; i < count; ++i)
                errors.push_back(static_cast<GenericSchemaValidator *>(subvalidators[i])->GetError(),
                                GetStateAllocator());
            currentError_.SetObject();
            currentError_.add_member(GetErrorsString(), errors, GetStateAllocator());
            AddCurrentError(code);
        }

        const SchemaType &CurrentSchema() const { return *schemaStack_.template Top<Context>()->schema; }

        Context &CurrentContext() { return *schemaStack_.template Top<Context>(); }

        const Context &CurrentContext() const { return *schemaStack_.template Top<Context>(); }

        static const size_t kDefaultSchemaStackCapacity = 1024;
        static const size_t kDefaultDocumentStackCapacity = 256;
        const SchemaDocumentType *schemaDocument_;
        const SchemaType &root_;
        StateAllocator *stateAllocator_;
        StateAllocator *ownStateAllocator_;
        internal::Stack<StateAllocator> schemaStack_;    //!< stack to store the current path of schema (BaseSchemaType *)
        internal::Stack<StateAllocator> documentStack_;  //!< stack to store the current path of validating document (char_type)
        OutputHandler *outputHandler_;
        ValueType error_;
        ValueType currentError_;
        ValueType missingDependents_;
        bool valid_;
        unsigned flags_;
        unsigned depth_;
    };

    typedef GenericSchemaValidator<SchemaDocument> SchemaValidator;

    ///////////////////////////////////////////////////////////////////////////////
    // SchemaValidatingReader

    //! A helper class for parsing with validation.
    /*!
        This helper class is a functor, designed as a parameter of \ref GenericDocument::populate().

        \tparam parseFlags Combination of \ref ParseFlag.
        \tparam InputStream Type of input stream, implementing Stream concept.
        \tparam SourceEncoding Encoding of the input stream.
        \tparam SchemaDocumentType Type of schema document.
        \tparam StackAllocator Allocator type for stack.
    */
    template<
            unsigned parseFlags,
            typename InputStream,
            typename SourceEncoding,
            typename SchemaDocumentType = SchemaDocument,
            typename StackAllocator = CrtAllocator>
    class SchemaValidatingReader {
    public:
        typedef typename SchemaDocumentType::PointerType PointerType;
        typedef typename InputStream::char_type char_type;
        typedef GenericValue<SourceEncoding, StackAllocator> ValueType;

        //! Constructor
        /*!
            \param is Input stream.
            \param sd Schema document.
        */
        SchemaValidatingReader(InputStream &is, const SchemaDocumentType &sd) : is_(is), sd_(sd),
                                                                                invalidSchemaKeyword_(),
                                                                                invalidSchemaCode_(kValidateErrorNone),
                                                                                error_(kObjectType), isValid_(true) {}

        template<typename Handler>
        bool operator()(Handler &handler) {
            GenericReader<SourceEncoding, typename SchemaDocumentType::EncodingType, StackAllocator> reader;
            GenericSchemaValidator <SchemaDocumentType, Handler> validator(sd_, handler);
            parseResult_ = reader.template parse<parseFlags>(is_, validator);

            isValid_ = validator.IsValid();
            if (isValid_) {
                invalidSchemaPointer_ = PointerType();
                invalidSchemaKeyword_ = 0;
                invalidDocumentPointer_ = PointerType();
                error_.SetObject();
            } else {
                invalidSchemaPointer_ = validator.GetInvalidSchemaPointer();
                invalidSchemaKeyword_ = validator.GetInvalidSchemaKeyword();
                invalidSchemaCode_ = validator.GetInvalidSchemaCode();
                invalidDocumentPointer_ = validator.GetInvalidDocumentPointer();
                error_.copy_from(validator.GetError(), allocator_);
            }

            return parseResult_;
        }

        const ParseResult &GetParseResult() const { return parseResult_; }

        bool IsValid() const { return isValid_; }

        const PointerType &GetInvalidSchemaPointer() const { return invalidSchemaPointer_; }

        const char_type *GetInvalidSchemaKeyword() const { return invalidSchemaKeyword_; }

        const PointerType &GetInvalidDocumentPointer() const { return invalidDocumentPointer_; }

        const ValueType &GetError() const { return error_; }

        ValidateErrorCode GetInvalidSchemaCode() const { return invalidSchemaCode_; }

    private:
        InputStream &is_;
        const SchemaDocumentType &sd_;

        ParseResult parseResult_;
        PointerType invalidSchemaPointer_;
        const char_type *invalidSchemaKeyword_;
        PointerType invalidDocumentPointer_;
        ValidateErrorCode invalidSchemaCode_;
        StackAllocator allocator_;
        ValueType error_;
        bool isValid_;
    };

}  // namespace merak::json
MERAK_JSON_DIAG_POP

#endif // MERAK_JSON_SCHEMA_H_
