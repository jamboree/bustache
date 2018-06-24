/*//////////////////////////////////////////////////////////////////////////////
    Copyright (c) 2016-2018 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////*/

#include <bustache/generate.hpp>
#include <bustache/generate/ostream.hpp>
#include <bustache/generate/string.hpp>

namespace bustache { namespace detail
{
    resolve_result content_visitor_base::resolve(std::string const& key, value_holder& hold) const
    {
        auto ki = key.begin();
        auto ke = key.end();
        if (ki == ke)
            return {};
        value_ptr pv = nullptr;
        if (*ki == '.')
        {
            if (++ki == ke)
                return {cursor, nullptr};
            auto k0 = ki;
            while (*ki != '.' && ++ki != ke);
            key_cache.assign(k0, ki);
            pv = scope->data.get(key_cache, hold);
        }
        else
        {
            auto k0 = ki;
            while (ki != ke && *ki != '.') ++ki;
            key_cache.assign(k0, ki);
            pv = scope->lookup(key_cache, hold);
        }
        return {pv, ki == ke ? nullptr : &*ki};
    }

    bool nested_resolver::next(iter ki, iter ke, value_holder& hold)
    {
        auto k0 = ++ki;
        while (ki != ke)
        {
            if (*ki == '.')
            {
                key_cache.assign(k0, ki);
                if (hold.used())
                {
                    value_holder new_hold;
                    obj = get_object(obj.get(key_cache, new_hold));
                    if (!obj)
                        return false;
                    return next(ki, ke, new_hold);
                }
                obj = get_object(obj.get(key_cache, hold));
                if (!obj)
                    return false;
                k0 = ++ki;
            }
            else
                ++ki;
        }
        key_cache.assign(k0, ki);
        if (hold.used())
        {
            value_holder new_hold;
            return done(new_hold);
        }
        return done(hold);
    }

    inline bool nested_resolver::done(value_holder& hold)
    {
        if (auto pv = obj.get(key_cache, hold))
        {
            handle(*pv);
            return true;
        }
        return false;
    }
}}

namespace bustache
{
    template
    void generate_ostream
    (
        std::ostream& out, format const& fmt,
        value_view const& data, detail::any_context const& context,
        option_type flag, default_unresolved_handler&&
    );

    template
    void generate_string
    (
        std::string& out, format const& fmt,
        value_view const& data, detail::any_context const& context,
        option_type flag, default_unresolved_handler&&
    );
}