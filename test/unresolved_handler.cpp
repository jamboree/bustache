/*//////////////////////////////////////////////////////////////////////////////
    Copyright (c) 2017-2020 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////*/
#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include <bustache/render/string.hpp>

using namespace bustache;

value_ptr throw_on_unresolved(std::string const& key)
{
    throw std::runtime_error("unresolved key: " + key);
}

value_ptr banana_on_unresolved(std::string const& key)
{
    static constexpr std::string_view banana("banana");
    return &banana;
}

TEST_CASE("unresolved")
{
    format const fmt("before-{{unresolved}}-after");

    std::string out;

    SECTION("throw")
    {
        CHECK_THROWS_WITH
        (
            render_string(out, fmt, nullptr, no_context, no_escape, throw_on_unresolved),
            "unresolved key: unresolved"
        );

        CHECK(out == "before-");
    }

    SECTION("default value")
    {
        render_string(out, fmt, nullptr, no_context, no_escape, banana_on_unresolved);

        CHECK(out == "before-banana-after");
    }
}