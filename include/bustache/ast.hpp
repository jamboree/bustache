/*//////////////////////////////////////////////////////////////////////////////
    Copyright (c) 2014-2016 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////*/
#ifndef BUSTACHE_AST_HPP_INCLUDED
#define BUSTACHE_AST_HPP_INCLUDED

#include "detail/variant.hpp"
#include <boost/utility/string_ref.hpp>
#include <vector>
#include <string>

namespace bustache { namespace ast
{
    struct variable;
    struct section;
    struct content;

    using text = boost::string_ref;

    using content_list = std::vector<content>;

    struct null {};

    struct variable
    {
        std::string key;
        char tag = '\0';
#ifdef _MSC_VER // Workaround MSVC bug.
        variable() = default;

        explicit variable(std::string key, char tag = '\0')
          : key(std::move(key)), tag(tag)
        {}
#endif
    };

    struct section
    {
        std::string key;
        content_list contents;
        char tag = '#';
    };

    struct partial
    {
        std::string key;
        std::string indent;
    };

#define BUSTACHE_AST_CONTENT(X, D)                                              \
    X(0, null, D)                                                               \
    X(1, text, D)                                                               \
    X(2, variable, D)                                                           \
    X(3, section, D)                                                            \
    X(4, partial, D)                                                            \
/***/

    struct content : detail::variant<content>
    {
        Zz_BUSTACHE_VARIANT_DECL(content, BUSTACHE_AST_CONTENT, true)

        content() noexcept : _which(0), _0() {}
    };
#undef BUSTACHE_AST_CONTENT

    inline bool is_null(content const& c)
    {
        return !c.which();
    }
}}

#endif