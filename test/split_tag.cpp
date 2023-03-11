/*//////////////////////////////////////////////////////////////////////////////
    Copyright (c) 2020-2023 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////*/
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <bustache/render/string.hpp>

using namespace bustache;

struct S
{
    bool b;
    int i;
    double f;
    std::string s;
};

template<>
struct bustache::impl_model<S>
{
    static constexpr model kind = model::object;
};

template<>
struct bustache::impl_object<S>
{
    static void get(S const& self, std::string const& key, value_handler visit)
    {
        if (key == "b")
            return visit(&self.b);
        if (key == "i")
            return visit(&self.i);
        if (key == "f")
            return visit(&self.f);
        if (key == "s")
            return visit(&self.s);
        return visit(nullptr);
    }
};

struct Sep
{
    char sep;
};

template<>
struct bustache::impl_model<Sep>
{
    static constexpr model kind = model::object;
};

template<>
struct bustache::impl_object<Sep>
{
    static void get(Sep self, std::string const& key, value_handler visit)
    {
        std::vector<std::string_view> v;
        auto i = key.data();
        auto i0 = i;
        auto const e = i + key.size();
        do
        {
            if (*i == self.sep)
            {
                v.push_back(std::string_view(i0, i - i0));
                i0 = i + 1;
            }
        } while (++i != e);
        v.push_back(std::string_view(i0, i - i0));
        return visit(&v);
    }
};

TEST_CASE("fmt-spec")
{
    S const s{true, 42, 3.1415, "hello"};
    CHECK(to_string("{{b:8}}"_fmt(s)) == "true    ");
    CHECK(to_string("{{i:8}}"_fmt(s)) == "      42");
    CHECK(to_string("{{f:.2f}}"_fmt(s)) == "3.14");
    CHECK(to_string("{{s:*>10}}"_fmt(s)) == "*****hello");
}

TEST_CASE("section-alias")
{
    Sep sep{'-'};
    CHECK(to_string("{{#a:over-my-dead-body}}({{.}}){{/a}}"_fmt(sep)) == "(over)(my)(dead)(body)");
}

TEST_CASE("parse-error")
{
    CHECK_THROWS_WITH("{{:}}"_fmt, "invalid key");
    CHECK_THROWS_WITH("{{:a}}"_fmt, "invalid key");
    CHECK_THROWS_WITH("{{a:}}"_fmt, "invalid key");
}