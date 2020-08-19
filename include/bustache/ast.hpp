/*//////////////////////////////////////////////////////////////////////////////
    Copyright (c) 2014-2020 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////*/
#ifndef BUSTACHE_AST_HPP_INCLUDED
#define BUSTACHE_AST_HPP_INCLUDED

#include <variant>
#include <unordered_map>
#include <vector>
#include <string>
#include <string_view>

namespace bustache::ast
{
    struct variable;
    struct section;
    struct content;

    using text = std::string_view;

    using content_list = std::vector<content>;

    using override_map = std::unordered_map<std::string, content_list>;

    struct null {};

    struct variable
    {
        std::string key;
        char tag = '\0';
    };

    struct block
    {
        std::string key;
        content_list contents;
    };

    struct section : block
    {
        char tag;
        explicit section(char tag = '#') noexcept : tag(tag) {}
    };

    struct inheritance : block {};

    struct partial
    {
        std::string key;
        std::string indent;
        override_map overriders;
    };

    struct content : std::variant<null, text, variable, section, partial, inheritance>
    {
        using variant::variant;

        bool is_null() const { return !index(); }
    };
}

#endif