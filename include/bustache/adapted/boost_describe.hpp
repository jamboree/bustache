/*//////////////////////////////////////////////////////////////////////////////
    Copyright (c) 2022 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////*/
#ifndef BUSTACHE_ADAPTED_BOOST_DESCRIBE_HPP_INCLUDED
#define BUSTACHE_ADAPTED_BOOST_DESCRIBE_HPP_INCLUDED

#include <bustache/model.hpp>
#include <boost/describe/bases.hpp>
#include <boost/describe/members.hpp>

namespace bustache::detail
{
    template<class T>
    using pub_members =
        boost::describe::describe_members<T, boost::describe::mod_public>;

    template<class T>
    using pub_bases =
        boost::describe::describe_bases<T, boost::describe::mod_public>;

    template<class T, template<class...> class L, class... D>
    bool visit_members(L<D...>, T const& self, std::string const& key, value_handler visit)
    {
        return (... || (key == D::name && (visit(&(self.*D::pointer)), true)));
    }

    template<class T>
    bool visit_udt(T const& self, std::string const& key, value_handler visit)
    {
        return visit_members(pub_members<T>{}, self, key, visit);
    }

    template<class T, template<class...> class L, class... D>
    bool visit_bases(L<D...>, T const& self, std::string const& key, value_handler visit)
    {
        return (... || visit_udt<typename D::type>(self, key, visit));
    }
}

namespace bustache
{
    template<class T>
    concept DescribedUDT = std::is_class_v<T> && requires {
        detail::pub_members<T>{};
        detail::pub_bases<T>{};
    };

    template<DescribedUDT T>
    struct bustache::impl_model<T>
    {
        static constexpr model kind = model::object;
    };

    template<DescribedUDT T>
    struct bustache::impl_object<T>
    {
        static void get(T const& self, std::string const& key, value_handler visit)
        {
            if (detail::visit_udt(self, key, visit))
                return;
            if (detail::visit_bases(detail::pub_bases<T>{}, self, key, visit))
                return;
            return visit(nullptr);
        }
    };
}

#endif