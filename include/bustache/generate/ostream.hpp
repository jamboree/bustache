/*//////////////////////////////////////////////////////////////////////////////
    Copyright (c) 2016-2020 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////*/
#ifndef BUSTACHE_GENERATE_OSTREAM_HPP_INCLUDED
#define BUSTACHE_GENERATE_OSTREAM_HPP_INCLUDED

#include <iostream>
#include <bustache/generate.hpp>

namespace bustache { namespace detail
{
    template<class CharT, class Traits>
    struct ostream_sink
    {
        std::basic_ostream<CharT, Traits>& out;

        void operator()(char const* it, char const* end) const
        {
            out.write(it, end - it);
        }

        template<class T>
        void operator()(T data) const
        {
            out << data;
        }

        void operator()(bool data) const
        {
            out << (data ? "true" : "false");
        }
    };
}}

namespace bustache
{
    template<class CharT, class Traits, class Context, class UnresolvedHandler = default_unresolved_handler>
    void generate_ostream
    (
        std::basic_ostream<CharT, Traits>& out, format const& fmt,
        value_view const& data, Context const& context,
        option_type flag, UnresolvedHandler&& f = {}
    )
    {
        detail::ostream_sink<CharT, Traits> sink{out};
        generate(sink, fmt, data, context, flag, std::forward<UnresolvedHandler>(f));
    }
    
    template<class CharT, class Traits, class T, class Context,
        typename std::enable_if_t<std::is_constructible_v<value_view, T>, bool> = true>
    inline std::basic_ostream<CharT, Traits>&
    operator<<(std::basic_ostream<CharT, Traits>& out, manipulator<T, Context> const& manip)
    {
        generate_ostream(out, manip.fmt, manip.data, context_view(manip.context), manip.flag);
        return out;
    }
}

#endif