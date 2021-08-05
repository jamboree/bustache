/*//////////////////////////////////////////////////////////////////////////////
    Copyright (c) 2016-2021 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////*/
#ifndef BUSTACHE_RENDER_STRING_HPP_INCLUDED
#define BUSTACHE_RENDER_STRING_HPP_INCLUDED

#include <string>
#include <bustache/render.hpp>

namespace bustache::detail
{
    template<class String>
    struct string_sink
    {
        String& out;

        void operator()(char const* data, std::size_t bytes) const
        {
            out.insert(out.end(), data, data + bytes);
        }
    };
}

namespace bustache
{
    template<class String, class Escape = no_escape_t>
    inline void render_string
    (
        String& out, format const& fmt,
        value_ref data, context_handler context = no_context_t{},
        Escape escape = {}, unresolved_handler f = nullptr
    )
    {
        render(detail::string_sink<String>{out}, fmt, data, context, escape, f);
    }
    
    template<class... Opts>
    inline std::string to_string(manipulator<Opts...> const& manip)
    {
        std::string ret;
        render_string(ret, manip.fmt, manip.data, get_context(manip), get_escape(manip));
        return ret;
    }
}

#endif