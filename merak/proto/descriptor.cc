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

