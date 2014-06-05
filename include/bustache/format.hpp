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
    template <typename Object, typename Context>
    struct manipulator
    {
        ast::content_list const& contents;
        Object const& data;
        Context const& context;
    };

    struct format
    {
        struct no_context
        {
            typedef std::pair<std::string, format> value_type;
            typedef value_type const* iterator;
            
            constexpr iterator find(std::string const&) const
            {
                return nullptr;
            }
            
            constexpr iterator end() const
            {
                return nullptr;
            }
    
            static no_context const& dummy()
            {
                static no_context const _{};
                return _;
            }
        };
        
        format(char const* begin, char const* end);

        template <typename Object>
        manipulator<Object, no_context>
        operator()(Object const& data) const
        {
            return {contents, data, no_context::dummy()};
        }
        
        template <typename Object, typename Context>
        manipulator<Object, Context>
        operator()(Object const& data, Context const& context) const
        {
            return {contents, data, context};
        }

        ast::content_list contents;
    };
}

#endif
