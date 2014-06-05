/*//////////////////////////////////////////////////////////////////////////////
    Copyright (c) 2014 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////*/
#ifndef BUSTACHE_AST_HPP_INCLUDED
#define BUSTACHE_AST_HPP_INCLUDED

#include <boost/variant.hpp>
#include <boost/fusion/include/define_struct_inline.hpp>
#include <boost/utility/string_ref.hpp>
#include <deque>
#include <string>

namespace bustache { namespace ast
{
    struct section;
    struct variable;

    using text = boost::string_ref;
    using partial = std::string;
    using comment = boost::blank;

    using content = boost::variant<comment, text, variable, section, partial>;

    using content_list = std::deque<content>;

    BOOST_FUSION_DEFINE_STRUCT_INLINE
    (
        variable,
        (char, tag)
        (std::string, id)
    )
    
    BOOST_FUSION_DEFINE_STRUCT_INLINE
    (
        section,
        (char, tag)
        (std::string, id)
        (content_list, contents)
    )
    
    inline bool is_null(content const& c)
    {
        return boost::get<boost::blank>(&c);
    }
}}

#endif
