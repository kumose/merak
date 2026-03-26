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

#include <merak/proto/descriptor.h>
#include <turbo/strings/match.h>

namespace merak {

    struct Instance {
        static const google::protobuf::DescriptorPool *get_pool() {
            static Instance ins;
            return ins.pool;
        }

    private:
        Instance() : pool(google::protobuf::DescriptorPool::generated_pool()) {

        }

        const google::protobuf::DescriptorPool *pool{nullptr};
    };

    using google::protobuf::Descriptor;
    using google::protobuf::FieldDescriptor;

    bool is_protobuf_map(const FieldDescriptor *field) {
        if (field->type() != FieldDescriptor::TYPE_MESSAGE || !field->is_repeated()) {
            return false;
        }
        const Descriptor *entry_desc = field->message_type();
        if (entry_desc == nullptr) {
            return false;
        }
        if (entry_desc->field_count() != 2) {
            return false;
        }
        const FieldDescriptor *key_desc = entry_desc->field(KEY_INDEX);
        if (nullptr == key_desc
            || key_desc->is_repeated()
            || key_desc->cpp_type() != FieldDescriptor::CPPTYPE_STRING
            || KEY_NAME !=  key_desc->name()) {
            return false;
        }
        const FieldDescriptor *value_desc = entry_desc->field(VALUE_INDEX);
        if (nullptr == value_desc
            || VALUE_NAME != value_desc->name()) {
            return false;
        }
        return true;
    }

    bool is_protobuf_any(const google::protobuf::FieldDescriptor* field) {
        if (field->type() != FieldDescriptor::TYPE_MESSAGE) {
            return false;
        }
        const Descriptor *entry_desc = field->message_type();
        if (entry_desc == nullptr) {
            return false;
        }
        if (entry_desc->field_count() != 2) {
            return false;
        }
        const FieldDescriptor *key_desc = entry_desc->field(KEY_INDEX);
        if (nullptr == key_desc
            || key_desc->is_repeated()
            || key_desc->cpp_type() != FieldDescriptor::CPPTYPE_STRING
            || TYPE_URL != key_desc->name()) {
            return false;
        }

        const FieldDescriptor *value_desc = entry_desc->field(VALUE_INDEX);
        if (nullptr == value_desc
            || VALUE_NAME != value_desc->name()) {
            return false;
        }

        return true;
    }


    google::protobuf::Message *create_message_by_type_name(std::string_view type_name) {
        const google::protobuf::Descriptor *descriptor = Instance::get_pool()->FindMessageTypeByName({type_name.data(), type_name.size()});

        if (!descriptor) {
            return nullptr;
        }

        const google::protobuf::Message *prototype =
                google::protobuf::MessageFactory::generated_factory()
                        ->GetPrototype(descriptor);

        if (!prototype) {
            return nullptr;
        }

        return prototype->New();
    }
}  // namespace merak

