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

#include "archiver.h"
#include <iostream>
#include <vector>

//////////////////////////////////////////////////////////////////////////////
// Test1: simple object

struct Student {
    Student() : name(), age(), height(), canSwim() {}
    Student(const std::string name, unsigned age, double height, bool canSwim) :
        name(name), age(age), height(height), canSwim(canSwim)
    {}

    std::string name;
    unsigned age;
    double height;
    bool canSwim;
};

template <typename Archiver>
Archiver& operator&(Archiver& ar, Student& s) {
    ar.start_object();
    ar.Member("name") & s.name;
    ar.Member("age") & s.age;
    ar.Member("height") & s.height;
    ar.Member("canSwim") & s.canSwim;
    return ar.end_object();
}

std::ostream& operator<<(std::ostream& os, const Student& s) {
    return os << s.name << " " << s.age << " " << s.height << " " << s.canSwim;
}

void test1() {
    std::string json;

    // Serialize
    {
        Student s("Lua", 9, 150.5, true);

        JsonWriter writer;
        writer & s;
        json = writer.get_string();
        std::cout << json << std::endl;
    }

    // Deserialize
    {
        Student s;
        JsonReader reader(json.c_str());
        reader & s;
        std::cout << s << std::endl;
    }
}

//////////////////////////////////////////////////////////////////////////////
// Test2: std::vector <=> JSON array
// 
// You can map a JSON array to other data structures as well

struct Group {
    Group() : groupName(), students() {}
    std::string groupName;
    std::vector<Student> students;
};

template <typename Archiver>
Archiver& operator&(Archiver& ar, Group& g) {
    ar.start_object();
    
    ar.Member("groupName");
    ar & g.groupName;

    ar.Member("students");
    size_t studentCount = g.students.size();
    ar.start_array(&studentCount);
    if (ar.IsReader)
        g.students.resize(studentCount);
    for (size_t i = 0; i < studentCount; i++)
        ar & g.students[i];
    ar.end_array();

    return ar.end_object();
}

std::ostream& operator<<(std::ostream& os, const Group& g) {
    os << g.groupName << std::endl;
    for (std::vector<Student>::const_iterator itr = g.students.begin(); itr != g.students.end(); ++itr)
        os << *itr << std::endl;
    return os;
}

void test2() {
    std::string json;

    // Serialize
    {
        Group g;
        g.groupName = "Rainbow";

        Student s1("Lua", 9, 150.5, true);
        Student s2("Mio", 7, 120.0, false);
        g.students.push_back(s1);
        g.students.push_back(s2);

        JsonWriter writer;
        writer & g;
        json = writer.get_string();
        std::cout << json << std::endl;
    }

    // Deserialize
    {
        Group g;
        JsonReader reader(json.c_str());
        reader & g;
        std::cout << g << std::endl;
    }
}

//////////////////////////////////////////////////////////////////////////////
// Test3: polymorphism & friend
//
// Note that friendship is not necessary but make things simpler.

class Shape {
public:
    virtual ~Shape() {}
    virtual const char* get_type() const = 0;
    virtual void Print(std::ostream& os) const = 0;

protected:
    Shape() : x_(), y_() {}
    Shape(double x, double y) : x_(x), y_(y) {}

    template <typename Archiver>
    friend Archiver& operator&(Archiver& ar, Shape& s);

    double x_, y_;
};

template <typename Archiver>
Archiver& operator&(Archiver& ar, Shape& s) {
    ar.Member("x") & s.x_;
    ar.Member("y") & s.y_;
    return ar;
}

class Circle : public Shape {
public:
    Circle() : radius_() {}
    Circle(double x, double y, double radius) : Shape(x, y), radius_(radius) {}
    ~Circle() {}

    const char* get_type() const { return "Circle"; }

    void Print(std::ostream& os) const {
        os << "Circle (" << x_ << ", " << y_ << ")" << " radius = " << radius_;
    }

private:
    template <typename Archiver>
    friend Archiver& operator&(Archiver& ar, Circle& c);

    double radius_;
};

template <typename Archiver>
Archiver& operator&(Archiver& ar, Circle& c) {
    ar & static_cast<Shape&>(c);
    ar.Member("radius") & c.radius_;
    return ar;
}

class Box : public Shape {
public:
    Box() : width_(), height_() {}
    Box(double x, double y, double width, double height) : Shape(x, y), width_(width), height_(height) {}
    ~Box() {}

    const char* get_type() const { return "Box"; }

    void Print(std::ostream& os) const {
        os << "Box (" << x_ << ", " << y_ << ")" << " width = " << width_ << " height = " << height_;
    }

private:
    template <typename Archiver>
    friend Archiver& operator&(Archiver& ar, Box& b);

    double width_, height_;
};

template <typename Archiver>
Archiver& operator&(Archiver& ar, Box& b) {
    ar & static_cast<Shape&>(b);
    ar.Member("width") & b.width_;
    ar.Member("height") & b.height_;
    return ar;
}

class Canvas {
public:
    Canvas() : shapes_() {}
    ~Canvas() { clear(); }
    
    void clear() {
        for (std::vector<Shape*>::iterator itr = shapes_.begin(); itr != shapes_.end(); ++itr)
            delete *itr;
    }

    void AddShape(Shape* shape) { shapes_.push_back(shape); }
    
    void Print(std::ostream& os) {
        for (std::vector<Shape*>::iterator itr = shapes_.begin(); itr != shapes_.end(); ++itr) {
            (*itr)->Print(os);
            std::cout << std::endl;
        }
    }

private:
    template <typename Archiver>
    friend Archiver& operator&(Archiver& ar, Canvas& c);

    std::vector<Shape*> shapes_;
};

template <typename Archiver>
Archiver& operator&(Archiver& ar, Shape*& shape) {
    std::string type = ar.IsReader ? "" : shape->get_type();
    ar.start_object();
    ar.Member("type") & type;
    if (type == "Circle") {
        if (ar.IsReader) shape = new Circle;
        ar & static_cast<Circle&>(*shape);
    }
    else if (type == "Box") {
        if (ar.IsReader) shape = new Box;
        ar & static_cast<Box&>(*shape);
    }
    return ar.end_object();
}

template <typename Archiver>
Archiver& operator&(Archiver& ar, Canvas& c) {
    size_t shapeCount = c.shapes_.size();
    ar.start_array(&shapeCount);
    if (ar.IsReader) {
        c.clear();
        c.shapes_.resize(shapeCount);
    }
    for (size_t i = 0; i < shapeCount; i++)
        ar & c.shapes_[i];
    return ar.end_array();
}

void test3() {
    std::string json;

    // Serialize
    {
        Canvas c;
        c.AddShape(new Circle(1.0, 2.0, 3.0));
        c.AddShape(new Box(4.0, 5.0, 6.0, 7.0));

        JsonWriter writer;
        writer & c;
        json = writer.get_string();
        std::cout << json << std::endl;
    }

    // Deserialize
    {
        Canvas c;
        JsonReader reader(json.c_str());
        reader & c;
        c.Print(std::cout);
    }
}

//////////////////////////////////////////////////////////////////////////////

int main() {
    test1();
    test2();
    test3();
}
