/*//////////////////////////////////////////////////////////////////////////////
    Copyright (c) 2014 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////*/
#ifndef BUSTACHE_FORMAT_HPP_INCLUDED
#define BUSTACHE_FORMAT_HPP_INCLUDED

#include "ast.hpp"

namespace bustache
{
    struct format
    {
        template <typename Object>
        struct manip
        {
            ast::content_list const& contents;
            Object const& data;
        };
        
        format(char const* begin, char const* end);

        template <typename Object>
        manip<Object> operator()(Object const& data) const
        {
            return {contents, data};
        }

        ast::content_list contents;
    };
}

#endif
