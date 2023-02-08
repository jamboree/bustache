/*//////////////////////////////////////////////////////////////////////////////
    Copyright (c) 2018-2023 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////*/
#include <catch2/catch_test_macros.hpp>
#include <bustache/render/string.hpp>

struct Inner
{
    int i32;
    std::string str;
};

struct Phantom {};

struct Outer
{
    Inner inner;
};

struct Range
{
    int a, b;
};

struct RangeRange : Range
{
    RangeRange(int a, int b) noexcept : Range{a, b} {}
};

template<>
struct bustache::impl_model<Inner>
{
    static constexpr model kind = model::object;
};

template<>
struct bustache::impl_object<Inner>
{
    static void get(Inner const& self, std::string const& key, value_handler visit)
    {
        if (key == "i32")
            return visit(&self.i32);
        if (key == "str")
            return visit(&self.str);
        return visit(nullptr);
    }
};

template<>
struct bustache::impl_model<Phantom>
{
    static constexpr model kind = model::object;
};

template<>
struct bustache::impl_object<Phantom>
{
    static void get(Phantom const&, std::string const& key, value_handler visit)
    {
        if (key == "age")
        {
            int age = 10;
            return visit(&age);
        }
        if (key == "name")
        {
            std::string_view name("Alice");
            return visit(&name);
        }
        return visit(nullptr);
    }
};

template<>
struct bustache::impl_model<Outer>
{
    static constexpr model kind = model::object;
};

template<>
struct bustache::impl_object<Outer>
{
    static void get(Outer const& self, std::string const& key, value_handler visit)
    {
        if (key == "inner")
            return visit(&self.inner);
        if (key == "phantom")
        {
            Phantom phantom;
            return visit(&phantom);
        }
        return visit(nullptr);
    }
};

template<>
struct bustache::impl_model<Range>
{
    static constexpr model kind = model::list;
};

template<>
struct bustache::impl_list<Range>
{
    static bool empty(Range const& self)
    {
        return self.a == self.b;
    }

    static void iterate(Range const& self, value_handler visit)
    {
        for (int i = self.a; i != self.b; ++i)
            visit(&i);
    }
};

template<>
struct bustache::impl_print<Range>
{
    static void print(Range const& self, output_handler os, char const*)
    {
        if (int i = self.a; i != self.b)
        {
            impl_print<int>::print(i, os, nullptr);
            while (++i != self.b)
            {
                os(",", 1);
                impl_print<int>::print(i, os, nullptr);
            }
        }
    }
};

template<>
struct bustache::impl_model<RangeRange>
{
    static constexpr model kind = model::list;
};

template<>
struct bustache::impl_list<RangeRange>
{
    static bool empty(RangeRange const& self)
    {
        return self.a == self.b;
    }

    static void iterate(RangeRange const& self, value_handler visit)
    {
        for (int i = self.a; i != self.b; ++i)
        {
            Range sub{1, i + 1};
            visit(&sub);
        }
    }
};

using namespace bustache;

TEST_CASE("custom_object")
{
    Outer outer{{42, "Ah-ha"}};

    CHECK(to_string(
        "inner:{{#inner}}(i32:{{i32}}, str:{{str}}){{/inner}}\n"
        "phantom:{{#phantom}}(name:{{name}}, age:{{age}}){{/phantom}}\n"_fmt(outer))
        ==
        "inner:(i32:42, str:Ah-ha)\n"
        "phantom:(name:Alice, age:10)\n");

    CHECK(to_string(
        "{{inner.i32}};{{inner.str}};{{phantom.name}};{{phantom.age}};"_fmt(outer))
        ==
        "42;Ah-ha;Alice;10;");
}

TEST_CASE("custom_list")
{
    Range range{1, 11};

    CHECK(to_string(
        "{{#.}}{{.}};{{/.}}"_fmt(range))
        ==
        "1;2;3;4;5;6;7;8;9;10;");

    CHECK(to_string(
        "{{.}}"_fmt(range))
        ==
        "1,2,3,4,5,6,7,8,9,10");

    CHECK(to_string(
        "{{^.}}nothing{{/.}}"_fmt(range))
        ==
        "");

    CHECK(to_string(
        "{{^.}}something{{/.}}"_fmt(Range{}))
        ==
        "something");


    CHECK(to_string(
        "{{#.}}{{.}}\n{{/.}}"_fmt(RangeRange{1, 4}))
        ==
        "1\n"
        "1,2\n"
        "1,2,3\n");
}