/*//////////////////////////////////////////////////////////////////////////////
    Copyright (c) 2020 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////*/
#ifndef BUSTACHE_MODEL_VARIANT_HPP_INCLUDED
#define BUSTACHE_MODEL_VARIANT_HPP_INCLUDED

#include <bustache/model.hpp>
#include <variant>

namespace bustache
{
    template<Value... T>
    struct impl_compatible<std::variant<T...>>
    {
        static value_ptr get_value_ptr(std::variant<T...> const& self)
        {
            return std::visit([](auto const& val) { return value_ptr(&val); }, self);
        }
    };
}

#endif