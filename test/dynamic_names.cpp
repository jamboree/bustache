/*//////////////////////////////////////////////////////////////////////////////
    Copyright (c) 2023 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////*/
#include <catch2/catch_test_macros.hpp>
#include <bustache/render/string.hpp>
#include "model.hpp"

using namespace bustache;
using namespace test;

TEST_CASE("dynamic-names") {
    // Basic Behavior - Partial
    CHECK(to_string("\"{{>*dynamic}}\""_fmt(object{{"dynamic", "content"}})
                        .context(context{{"content", "Hello, world!"_fmt}})) ==
          "\"Hello, world!\"");

    // Basic Behavior - Name Resolution
    CHECK(to_string("\"{{>*dynamic}}\""_fmt(
                        object{{"dynamic", "content"}, {"*dynamic", "wrong"}})
                        .context(context{{"content", "Hello, world!"_fmt},
                                         {"wrong", "Invisible"_fmt}})) ==
          "\"Hello, world!\"");

    // Context Misses - Partial
    CHECK(to_string("\"{{>*missing}}\""_fmt(object{}).context(
              context{{"missing", "Hello, world!"_fmt}})) == "\"\"");

    // Failed Lookup - Partial
    CHECK(to_string("\"{{>*dynamic}}\""_fmt(object{{"dynamic", "content"}})
                        .context(context{{"foobar", "Hello, world!"_fmt}})) ==
          "\"\"");

    // Context
    CHECK(to_string("\"{{>*example}}\""_fmt(object{{"text", "Hello, world!"},
                                                   {"example", "partial"}})
                        .context(context{{"partial", "*{{text}}*"_fmt}})) ==
          "\"*Hello, world!*\"");

    // Dotted Names
    CHECK(to_string(
              "\"{{>*foo.bar.baz}}\""_fmt(
                  object{{"text", "Hello, world!"},
                         {"foo", object{{"bar", object{{"baz", "partial"}}}}}})
                  .context(context{{"partial", "*{{text}}*"_fmt}})) ==
          "\"*Hello, world!*\"");

    // Dotted Names - Operator Precedence
    CHECK(to_string(
              "\"{{>*foo.bar.baz}}\""_fmt(
                  object{{"text", "Hello, world!"},
                         {"foo", "test"},
                         {"test", object{{"bar", object{{"baz", "partial"}}}}}})
                  .context(context{{"partial", "*{{text}}*"_fmt}})) == "\"\"");

    // "Dotted Names - Failed Lookup"
    CHECK(
        to_string("\"{{>*foo.bar.baz}}\""_fmt(
                      object{
                          {"foo", object{{"text", "Hello, world!"},
                                         {"bar", object{{"baz", "partial"}}}}},
                      })
                      .context(context{{"partial", "*{{text}}*"_fmt}})) ==
        "\"**\"");

    // Dotted names - Context Stacking
    CHECK(to_string("{{#section1}}{{>*section2.dynamic}}{{/section1}}"_fmt(
                        object{{"section1", object{{"value", "section1"}}},
                               {"section2", object{{"dynamic", "partial"},
                                                   {"value", "section2"}}}})
                        .context(context{{"partial", "\"{{value}}\""_fmt}})) ==
          "\"section1\"");

    // Dotted names - Context Stacking Under Repetition
    CHECK(to_string("{{#section1}}{{>*section2.dynamic}}{{/section1}}"_fmt(
                        object{{"value", "test"},
                               {"section1", array{1, 2}},
                               {"section2", object{{"dynamic", "partial"},
                                                   {"value", "section2"}}}})
                        .context(context{{"partial", "{{value}}"_fmt}})) ==
          "testtest");

    // Dotted names - Context Stacking Failed Lookup
    CHECK(to_string("{{#section1}}{{>*section2.dynamic}}{{/section1}}"_fmt(
                        object{{"section1", array{1, 2}},
                               {"section2", object{{"dynamic", "partial"},
                                                   {"value", "section2"}}}})
                        .context(context{{"partial", "\"{{value}}\""_fmt}})) ==
          "\"\"\"\"");

    // Recursion
    CHECK(
        to_string(
            "{{>*template}}"_fmt(
                object{{"template", "node"},
                       {"content", "X"},
                       {"nodes",
                        array{object{{"content", "Y"}, {"nodes", array{}}}}}})
                .context(context{
                    {"node",
                     "{{content}}<{{#nodes}}{{>*template}}{{/nodes}}>"_fmt}})) ==
        "X<Y<>>");

    // Dynamic Names - Double Dereferencing
    CHECK(to_string("\"{{>**dynamic}}\""_fmt(
                        object{{"dynamic", "test"}, {"test", "content"}})
                        .context(context{{"content", "Hello, world!"_fmt}})) ==
          "\"\"");

    // Dynamic Names - Composed Dereferencing
    CHECK(
        to_string("\"{{>*foo.*bar}}\""_fmt(
                      object{{"foo", "fizz"},
                             {"bar", "buzz"},
                             {"fizz",
                              object{{"buzz", object{{"content", object{}}}}}}})
                      .context(context{{"content", "Hello, world!"_fmt}})) ==
        "\"\"");

    // Surrounding Whitespace
    CHECK(to_string("| {{>*partial}} |"_fmt(object{{"partial", "foobar"}})
                        .context(context{{"foobar", "\t|\t"_fmt}})) ==
          "| \t|\t |");

    // Inline Indentation
    CHECK(to_string("  {{data}}  {{>*dynamic}}\n"_fmt(
                        object{{"dynamic", "partial"}, {"data", "|"}})
                        .context(context{{"partial", ">\n>"_fmt}})) ==
          "  |  >\n>\n");

    // Standalone Line Endings
    CHECK(
        to_string("|\r\n{{>*dynamic}}\r\n|"_fmt(object{{"dynamic", "partial"}})
                      .context(context{{"partial", ">"_fmt}})) == "|\r\n>|");

    // Standalone Without Previous Line
    CHECK(to_string("  {{>*dynamic}}\n>"_fmt(object{{"dynamic", "partial"}})
                        .context(context{{"partial", ">\n>"_fmt}})) ==
          "  >\n  >>");

    // Standalone Without Newline
    CHECK(to_string(">\n  {{>*dynamic}}"_fmt(object{{"dynamic", "partial"}})
                        .context(context{{"partial", ">\n>"_fmt}})) ==
          ">\n  >\n  >");

    // Standalone Indentation
    CHECK(to_string("\\\n {{>*dynamic}}\n/\n"_fmt(
                        object{{"dynamic", "partial"}, {"content", "<\n->"}})
                        .context(context{
                            {"partial", "|\n{{{content}}}\n|\n"_fmt}})) ==
          "\\\n |\n <\n->\n |\n/\n");

    // Padding Whitespace
    CHECK(to_string("|{{> * dynamic }}|"_fmt(
                        object{{"dynamic", "partial"}, {"boolean", true}})
                        .context(context{{"partial", "[]"_fmt}})) == "|[]|");
}