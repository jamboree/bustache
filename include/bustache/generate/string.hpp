/*//////////////////////////////////////////////////////////////////////////////
    Copyright (c) 2016-2020 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////*/
#ifndef BUSTACHE_GENERATE_STRING_HPP_INCLUDED
#define BUSTACHE_GENERATE_STRING_HPP_INCLUDED

#include <charconv>
#include <string>
#include <bustache/generate.hpp>

namespace bustache { namespace detail
{
    template<class String>
    struct string_sink
    {
        String& out;

        void operator()(char const* it, char const* end) const
        {
            out.insert(out.end(), it, end);
        }

        template<class T>
        void operator()(T data) const
        {
            char buf[1024];
            auto const [p, ec] = std::to_chars(buf, buf + sizeof(buf), data);
            if (ec == std::errc())
                out.insert(out.end(), buf, p);
        }

        void operator()(bool data) const
        {
            data ? append("true") : append("false");
        }

        void append(strlit str) const
        {
            out.insert(out.end(), str.data, str.data + str.size);
        }
    };
}}

namespace bustache
{
    template<class String, class Context, class UnresolvedHandler = default_unresolved_handler>
    void generate_string
    (
        String& out, format const& fmt,
        value_view const& data, Context const& context,
        option_type flag, UnresolvedHandler&& f = {}
    )
    {
        detail::string_sink<String> sink{out};
        generate(sink, fmt, data, context, flag, std::forward<UnresolvedHandler>(f));
    }
    
    template<class T, class Context,
        typename std::enable_if_t<std::is_constructible_v<value_view, T>, bool> = true>
    inline std::string to_string(manipulator<T, Context> const& manip)
    {
        std::string ret;
        generate_string(ret, manip.fmt, manip.data, context_view(manip.context), manip.flag);
        return ret;
    }
}

#endif