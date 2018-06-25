/*//////////////////////////////////////////////////////////////////////////////
    Copyright (c) 2018 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////*/
#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include <bustache/model.hpp>

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

namespace bustache
{
    template<>
    struct object_trait<Inner>
    {
        static value_ptr get(Inner const& self, std::string const& key, value_holder&)
        {
            if (key == "i32")
                return value_view(self.i32).get_pointer();
            if (key == "str")
                return value_view(self.str).get_pointer();
            return nullptr;
        }
    };

    template<>
    struct object_trait<Phantom>
    {
        static value_ptr get(Phantom const&, std::string const& key, value_holder& hold)
        {
            if (key == "age")
                return hold(10);
            if (key == "name")
                return hold("Alice");
            return nullptr;
        }
    };

    template<>
    struct object_trait<Outer>
    {
        static value_ptr get(Outer const& self, std::string const& key, value_holder& hold)
        {
            if (key == "inner")
                return hold(object_view(self.inner));
            if (key == "phantom")
                return hold.object(Phantom{});
            return nullptr;
        }
    };

    template<>
    struct list_trait<Range>
    {
        static bool empty(Range const& self)
        {
            return self.a == self.b;
        }

        static std::uintptr_t begin_cursor(Range const&)
        {
            return 0;
        }

        static value_ptr next(Range const& self, std::uintptr_t& state, value_holder& hold)
        {
            auto i = self.a + int(state);
            if (i == self.b)
                return nullptr;
            ++state;
            return hold(i);
        }

        static void end_cursor(Range const&, std::uintptr_t) noexcept {}
    };

    template<>
    struct list_trait<RangeRange> : list_trait<Range>
    {
        static value_ptr next(RangeRange const& self, std::uintptr_t& state, value_holder& hold)
        {
            auto i = self.a + int(state);
            if (i == self.b)
                return nullptr;
            ++state;
            return hold.list(Range{1, i + 1});
        }
    };
}

using namespace bustache;

TEST_CASE("custom_object")
{
    Outer outer{{42, "Ah-ha"}};

    CHECK(to_string(
        "inner:{{#inner}}(i32:{{i32}}, str:{{str}}){{/inner}}\n"
        "phantom:{{#phantom}}(name:{{name}}, age:{{age}}){{/phantom}}\n"_fmt(object_view(outer)))
        ==
        "inner:(i32:42, str:Ah-ha)\n"
        "phantom:(name:Alice, age:10)\n");

    CHECK(to_string(
        "{{inner.i32}};{{inner.str}};{{phantom.name}};{{phantom.age}};"_fmt(object_view(outer)))
        ==
        "42;Ah-ha;Alice;10;");
}

TEST_CASE("custom_list")
{
    Range range{1, 11};

    CHECK(to_string(
        "{{#.}}{{.}};{{/.}}"_fmt(list_view(range)))
        ==
        "1;2;3;4;5;6;7;8;9;10;");

    CHECK(to_string(
        "{{.}}"_fmt(list_view(range)))
        ==
        "1,2,3,4,5,6,7,8,9,10");

    CHECK(to_string(
        "{{^.}}nothing{{/.}}"_fmt(list_view(range)))
        ==
        "");

    CHECK(to_string(
        "{{^.}}something{{/.}}"_fmt(list_view(Range{})))
        ==
        "something");


    CHECK(to_string(
        "{{#.}}{{.}}\n{{/.}}"_fmt(list_view(RangeRange{1, 4})))
        ==
        "1\n"
        "1,2\n"
        "1,2,3\n");
}