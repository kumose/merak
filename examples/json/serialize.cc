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
// Serialize example
// This example shows writing JSON string with writer directly.

#include <merak/json.h>
#include <cstdio>
#include <string>
#include <vector>

using namespace merak::json;

class Person {
public:
    Person(const std::string& name, unsigned age) : name_(name), age_(age) {}
    Person(const Person& rhs) : name_(rhs.name_), age_(rhs.age_) {}
    virtual ~Person();

    Person& operator=(const Person& rhs) {
        name_ = rhs.name_;
        age_ = rhs.age_;
        return *this;
    }

protected:
    template <typename Writer>
    void Serialize(Writer& writer) const {
        // This base class just write out name-value pairs, without wrapping within an object.
        writer.emplace_string("name");
        writer.emplace_string(name_);
        writer.emplace_string("age");
        writer.emplace_uint32(age_);
    }

private:
    std::string name_;
    unsigned age_;
};

Person::~Person() {
}

class Education {
public:
    Education(const std::string& school, double GPA) : school_(school), GPA_(GPA) {}
    Education(const Education& rhs) : school_(rhs.school_), GPA_(rhs.GPA_) {}

    template <typename Writer>
    void Serialize(Writer& writer) const {
        writer.start_object();
        
        writer.emplace_string("school");
        writer.emplace_string(school_);

        writer.emplace_string("GPA");
        writer.emplace_double(GPA_);

        writer.end_object();
    }

private:
    std::string school_;
    double GPA_;
};

class Dependent : public Person {
public:
    Dependent(const std::string& name, unsigned age, Education* education = 0) : Person(name, age), education_(education) {}
    Dependent(const Dependent& rhs) : Person(rhs), education_(0) { education_ = (rhs.education_ == 0) ? 0 : new Education(*rhs.education_); }
    virtual ~Dependent();

    Dependent& operator=(const Dependent& rhs) {
        if (this == &rhs)
            return *this;
        delete education_;
        education_ = (rhs.education_ == 0) ? 0 : new Education(*rhs.education_);
        return *this;
    }

    template <typename Writer>
    void Serialize(Writer& writer) const {
        writer.start_object();

        Person::Serialize(writer);

        writer.emplace_string("education");
        if (education_)
            education_->Serialize(writer);
        else
            writer.emplace_null();

        writer.end_object();
    }

private:

    Education *education_;
};

Dependent::~Dependent() {
    delete education_; 
}

class Employee : public Person {
public:
    Employee(const std::string& name, unsigned age, bool married) : Person(name, age), dependents_(), married_(married) {}
    Employee(const Employee& rhs) : Person(rhs), dependents_(rhs.dependents_), married_(rhs.married_) {}
    virtual ~Employee();

    Employee& operator=(const Employee& rhs) {
        static_cast<Person&>(*this) = rhs;
        dependents_ = rhs.dependents_;
        married_ = rhs.married_;
        return *this;
    }

    void AddDependent(const Dependent& dependent) {
        dependents_.push_back(dependent);
    }

    template <typename Writer>
    void Serialize(Writer& writer) const {
        writer.start_object();

        Person::Serialize(writer);

        writer.emplace_string("married");
        writer.emplace_bool(married_);

        writer.emplace_string(("dependents"));
        writer.start_array();
        for (std::vector<Dependent>::const_iterator dependentItr = dependents_.begin(); dependentItr != dependents_.end(); ++dependentItr)
            dependentItr->Serialize(writer);
        writer.end_array();

        writer.end_object();
    }

private:
    std::vector<Dependent> dependents_;
    bool married_;
};

Employee::~Employee() {
}

int main(int, char*[]) {
    std::vector<Employee> employees;

    employees.push_back(Employee("Milo YIP", 34, true));
    employees.back().AddDependent(Dependent("Lua YIP", 3, new Education("Happy Kindergarten", 3.5)));
    employees.back().AddDependent(Dependent("Mio YIP", 1));

    employees.push_back(Employee("Percy TSE", 30, false));

    StringBuffer sb;
    PrettyWriter<StringBuffer> writer(sb);

    writer.start_array();
    for (std::vector<Employee>::const_iterator employeeItr = employees.begin(); employeeItr != employees.end(); ++employeeItr)
        employeeItr->Serialize(writer);
    writer.end_array();

    puts(sb.get_string());

    return 0;
}
