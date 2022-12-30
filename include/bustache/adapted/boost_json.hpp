/*//////////////////////////////////////////////////////////////////////////////
    Copyright (c) 2022 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////*/
#ifndef BUSTACHE_ADAPTED_BOOST_JSON_HPP_INCLUDED
#define BUSTACHE_ADAPTED_BOOST_JSON_HPP_INCLUDED

#include <bustache/model.hpp>
#include <boost/json/value.hpp>

namespace bustache
{
    template<>
    struct bustache::impl_model<boost::json::object>
    {
        static constexpr model kind = model::object;
    };

    template<>
    struct impl_compatible<boost::json::value>
    {
        static value_ptr get_value_ptr(boost::json::value const& self)
        {
            // Use non-const version for reference.
            auto& ref = const_cast<boost::json::value&>(self);
            switch (ref.kind())
            {
            case boost::json::kind::null:
                break;
            case boost::json::kind::bool_:
                return value_ptr(&ref.get_bool());
            case boost::json::kind::int64:
                return value_ptr(&ref.get_int64());
            case boost::json::kind::uint64:
                return value_ptr(&ref.get_uint64());
            case boost::json::kind::double_:
                return value_ptr(&ref.get_double());
            case boost::json::kind::string:
                return value_ptr(&ref.get_string());
            case boost::json::kind::array:
                return value_ptr(&ref.get_array());
            case boost::json::kind::object:
                return value_ptr(&ref.get_object());
            }
            return value_ptr();
        }
    };

    template<>
    struct bustache::impl_object<boost::json::object>
    {
        static void get(boost::json::object const& self, std::string const& key, value_handler visit)
        {
            auto const it = self.find(key);
            visit(it == self.end() ? nullptr : &it->value());
        }
    };
}

#endif