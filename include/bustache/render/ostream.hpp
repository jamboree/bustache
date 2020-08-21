/*//////////////////////////////////////////////////////////////////////////////
    Copyright (c) 2016-2020 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////*/
#ifndef BUSTACHE_RENDER_OSTREAM_HPP_INCLUDED
#define BUSTACHE_RENDER_OSTREAM_HPP_INCLUDED

#include <iostream>
#include <bustache/render.hpp>

namespace bustache { namespace detail
{
    template<class CharT, class Traits>
    struct ostream_sink
    {
        std::basic_ostream<CharT, Traits>& out;

        void operator()(char const* data, std::size_t bytes) const
        {
            out.write(data, bytes);
        }
    };
}}

namespace bustache
{
    template<class CharT, class Traits, Value T, class Escape = no_escape_t>
    inline void render_ostream
    (
        std::basic_ostream<CharT, Traits>& out, format const& fmt,
        T const& data, context_handler context = no_context_t{},
        Escape escape = {}, unresolved_handler f = nullptr
    )
    {
        render(detail::ostream_sink<CharT, Traits>{out}, fmt, data, context, escape, f);
    }
    
    template<class CharT, class Traits, class... Opts>
    inline std::basic_ostream<CharT, Traits>&
    operator<<(std::basic_ostream<CharT, Traits>& out, manipulator<Opts...> const& manip)
    {
        render_ostream(out, manip.fmt, manip.data, detail::get_context(&manip), detail::get_escape(&manip));
        return out;
    }
}

#endif