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
    
    enum option_type
    {
        trim_ws = 1
    };

    struct format
    {
        typedef bool flag_type;
        
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
        
        format(char const* begin, char const* end, flag_type flag = 0);
        
        template <typename Source>
        explicit format(Source const& source, flag_type flag = 0)
          : format(source.data(), source.data() + source.size(), flag)
        {}
        
        template <typename Source>
        explicit format(Source const&& source, flag_type flag = 0)
          : format(source.data(), source.data() + source.size(), flag)
        {
            copy_text(text_size());
        }

        explicit format(std::string&& source, flag_type flag = 0)
          : format(source.data(), source.data() + source.size(), flag)
        {
            std::size_t sn = source.size(), tn;
            if (sn > 1024 && sn - (tn = text_size()) > 256)
                copy_text(tn);
            else
                _text.swap(source);
        }

        template <std::size_t N>
        format(char const (&source)[N], flag_type flag = 0)
          : format(source, source + (N - 1), flag)
        {}
        
        template <typename Object>
        manipulator<Object, no_context>
        operator()(Object const& data) const
        {
            return {_contents, data, no_context::dummy()};
        }
        
        template <typename Object, typename Context>
        manipulator<Object, Context>
        operator()(Object const& data, Context const& context) const
        {
            return {_contents, data, context};
        }
        
        ast::content_list const& contents() const
        {
            return _contents;
        }
        
    private:
        
        std::size_t text_size() const;
        void copy_text(std::size_t n);

        ast::content_list _contents;
        std::string _text;
    };
}

#endif
