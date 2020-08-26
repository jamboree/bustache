/*//////////////////////////////////////////////////////////////////////////////
    Copyright (c) 2020 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////*/
#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include <bustache/render/string.hpp>
#include "model.hpp"

using namespace bustache;
using namespace test;

TEST_CASE("inheritance")
{
    // Default
    CHECK(to_string("{{$title}}Default title{{/title}}"_fmt(nullptr)) == "Default title");

    // Variable
    CHECK(to_string("{{$foo}}default {{bar}} content{{/foo}}"_fmt(object{{"bar", "baz"}})) == "default baz content");

    // Triple Mustache
    CHECK(to_string("{{$foo}}default {{{bar}}} content{{/foo}}"_fmt(object{{"bar", "<baz>"}}).escape(escape_html)) == "default <baz> content");

    // Sections
    CHECK(to_string("{{$foo}}default {{#bar}}{{baz}}{{/bar}} content{{/foo}}"_fmt(object{{"bar", object{{"baz", "qux"}}}})) == "default qux content");

    // Negative Sections
    CHECK(to_string("{{$foo}}default {{^bar}}{{baz}}{{/bar}} content{{/foo}}"_fmt(object{{"baz", "three"}})) == "default three content");

    // Mustache Injection
    CHECK(to_string("{{$foo}}default {{#bar}}{{baz}}{{/bar}} content{{/foo}}"_fmt(object{{"bar", object{{"baz", "{{qux}}"}}}})) == "default {{qux}} content");

    // Inherit
    CHECK(to_string("{{<include}}{{/include}}"_fmt(nullptr).context(context{{"include", "{{$foo}}default content{{/foo}}"_fmt}})) == "default content");

    // Overridden content
    CHECK(to_string("{{<super}}{{$title}}sub template title{{/title}}{{/super}}"_fmt(nullptr)
        .context(context{{"super", "...{{$title}}Default title{{/title}}..."_fmt}})) == "...sub template title...");

    // Data does not override block
    CHECK(to_string("{{<include}}{{$var}}var in template{{/var}}{{/include}}"_fmt(object{{"var", "var in data"}})
        .context(context{{"include", "{{$var}}var in include{{/var}}"_fmt}})) == "var in template");

    // Data does not override block default
    CHECK(to_string("{{<include}}{{/include}}"_fmt(object{{"var", "var in data"}})
        .context(context{{"include", "{{$var}}var in include{{/var}}"_fmt}})) == "var in include");

    // Overridden partial
    CHECK(to_string("test {{<partial}}{{$stuff}}override{{/stuff}}{{/partial}}"_fmt(nullptr)
        .context(context{{"partial", "{{$stuff}}...{{/stuff}}"_fmt}})) == "test override");

    // Two overridden partials
    CHECK(to_string("test {{<partial}}{{$stuff}}override1{{/stuff}}{{/partial}} {{<partial}}{{$stuff}}override2{{/stuff}}{{/partial}}"_fmt(nullptr)
        .context(context{{"partial", "|{{$stuff}}...{{/stuff}}{{$default}} default{{/default}}|"_fmt}})) == "test |override1 default| |override2 default|");

    // Override partial with newlines
    CHECK(to_string("{{<partial}}{{$ballmer}}\npeaked\n\n:(\n{{/ballmer}}{{/partial}}"_fmt(nullptr)
        .context(context{{"partial", "{{$ballmer}}peaking{{/ballmer}}"_fmt}})) == "peaked\n\n:(\n");

    // Inherit indentation
    CHECK(to_string("{{<partial}}{{$nineties}}hammer time{{/nineties}}{{/partial}}"_fmt(nullptr)
        .context(context{{"partial", "stop:\n    {{$nineties}}collaborate and listen{{/nineties}}"_fmt}})) == "stop:\n    hammer time");

    // Only one override
    CHECK(to_string("{{<partial}}{{$stuff2}}override two{{/stuff2}}{{/partial}}"_fmt(nullptr)
        .context(context{{"partial", "{{$stuff}}new default one{{/stuff}}, {{$stuff2}}new default two{{/stuff2}}"_fmt}})) == "new default one, override two");

    // Super template
    CHECK(to_string("{{>include}}|{{<include}}{{/include}}"_fmt(nullptr)
        .context(context{{"include", "{{$foo}}default content{{/foo}}"_fmt}})) == "default content|default content");

    // Recursion
    CHECK(to_string("{{<include}}{{$foo}}override{{/foo}}{{/include}}"_fmt(nullptr)
        .context(context{
            {"include", "{{$foo}}default content{{/foo}} {{$bar}}{{<include2}}{{/include2}}{{/bar}}"_fmt},
            {"include2", "{{$foo}}include2 default content{{/foo}} {{<include}}{{$bar}}don't recurse{{/bar}}{{/include}}"_fmt}
        })) == "override override override don't recurse");

    // Multi-level inheritance
    CHECK(to_string("{{<parent}}{{$a}}c{{/a}}{{/parent}}"_fmt(nullptr)
        .context(context{
            {"parent", "{{<older}}{{$a}}p{{/a}}{{/older}}"_fmt},
            {"older", "{{<grandParent}}{{$a}}o{{/a}}{{/grandParent}}"_fmt},
            {"grandParent", "{{$a}}g{{/a}}"_fmt}
            })) == "c");

    // Multi-level inheritance, no sub child
    CHECK(to_string("{{<parent}}{{/parent}}"_fmt(nullptr)
        .context(context{
            {"parent", "{{<older}}{{$a}}p{{/a}}{{/older}}"_fmt},
            {"older", "{{<grandParent}}{{$a}}o{{/a}}{{/grandParent}}"_fmt},
            {"grandParent", "{{$a}}g{{/a}}"_fmt}
            })) == "p");

    // Text inside super
    CHECK(to_string("{{<include}} asdfasd {{$foo}}hmm{{/foo}} asdfasdfasdf {{/include}}"_fmt(nullptr)
        .context(context{{"include", "{{$foo}}default content{{/foo}}"_fmt}})) == "hmm");

    // Text inside super
    CHECK(to_string("{{<include}} asdfasd asdfasdfasdf {{/include}}"_fmt(nullptr)
        .context(context{{"include", "{{$foo}}default content{{/foo}}"_fmt}})) == "default content");
}