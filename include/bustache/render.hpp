/*//////////////////////////////////////////////////////////////////////////////
    Copyright (c) 2016-2021 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////*/
#ifndef BUSTACHE_RENDER_HPP_INCLUDED
#define BUSTACHE_RENDER_HPP_INCLUDED

#include <bustache/model.hpp>

namespace bustache
{
    using unresolved_handler = fn_ptr<value_ptr(std::string const&)>;

    using context_handler = fn_ref<format const*(std::string const&)>;

    struct no_context_t
    {
        format const* operator()(std::string const&) const
        {
            return nullptr;
        }
    };

    constexpr no_context_t no_context{};

    template<class Map>
    struct map_context
    {
        Map const& map;

        map_context(Map const& map) noexcept : map(map) {}

        format const* operator()(std::string const& key) const
        {
            auto it = map.find(key);
            return it == map.end() ? nullptr : &it->second;
        }
    };

    namespace detail
    {
        inline no_context_t get_context(void const*)
        {
            return {};
        }

        template<class T>
        inline T const& get_context(manip_context<T> const* p)
        {
            return p->context;
        }
    }

    template<class... Opts>
    inline decltype(auto) get_context(manipulator<Opts...> const& manip)
    {
        return detail::get_context(&manip);
    }
}

namespace bustache::detail
{
    struct strlit
    {
        char const* data;
        std::size_t size;

        constexpr strlit() : data(), size() {}

        template<std::size_t N>
        constexpr strlit(char const (&str)[N]) : data(str), size(N - 1) {}

        constexpr explicit operator bool() const
        {
            return !!data;
        }
    };

    inline strlit get_escaped(char c) noexcept
    {
        switch (c)
        {
        case '&': return "&amp;";
        case '<': return "&lt;";
        case '>': return "&gt;";
        case '\\': return "&#92;";
        case '"': return "&quot;";
        default:  return {};
        }
    }

    template<class Sink>
    struct escape_sink
    {
        Sink& sink;

        void operator()(const void* data, std::size_t bytes) const
        {
            auto it = static_cast<char const*>(data);
            auto last = it;
            auto const end = it + bytes;
            while (it != end)
            {
                if (auto const str = get_escaped(*it))
                {
                    sink(last, it - last);
                    sink(str.data, str.size);
                    last = ++it;
                }
                else
                    ++it;
            }
            sink(last, it - last);
        }
    };

    BUSTACHE_API void render
    (
        output_handler raw_os, output_handler escape_os, format const& fmt, value_ptr data,
        context_handler context, unresolved_handler f
    );
}

namespace bustache
{
    struct no_escape_t
    {
        output_handler operator()(output_handler os) const
        {
            return os;
        }
    };

    constexpr no_escape_t no_escape{};

    constexpr auto escape_html = []<class Sink>(Sink& sink)
    {
        return detail::escape_sink<Sink>{sink};
    };

    namespace detail
    {
        inline no_escape_t get_escape(void const*)
        {
            return {};
        }

        template<class T>
        inline T const& get_escape(detail::manip_escape<T> const* p)
        {
            return p->escape;
        }
    }

    template<class... Opts>
    inline decltype(auto) get_escape(manipulator<Opts...> const& manip)
    {
        return detail::get_escape(&manip);
    }

    template<class Sink, class Escape = no_escape_t>
    inline void render
    (
        Sink&& os, format const& fmt, value_ref data,
        context_handler context = no_context_t{}, Escape escape = {},
        unresolved_handler f = nullptr
    )
    {
        detail::render(os, escape(os), fmt, data.get_ptr(), context, f);
    }
}

#endif