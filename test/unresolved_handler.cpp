/*//////////////////////////////////////////////////////////////////////////////
    Copyright (c) 2017-2018 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////*/
#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include <bustache/generate/string.hpp>

using namespace bustache;

value throw_on_unresolved(std::string const& key)
{
    throw std::runtime_error("unresolved key: " + key);
}

value banana_on_unresolved(std::string const& key)
{
    return "banana";
}

TEST_CASE("unresolved")
{
    format const fmt("before-{{unresolved}}-after");
    object const empty;

    std::string out;

    SECTION("throw")
    {
        CHECK_THROWS_WITH
        (
            generate_string(out, fmt, empty, no_context, normal, throw_on_unresolved),
            "unresolved key: unresolved"
        );

        CHECK(out == "before-");
    }

    SECTION("default value")
    {
        generate_string(out, fmt, empty, no_context, normal, banana_on_unresolved);

        CHECK(out == "before-banana-after");
    }
}