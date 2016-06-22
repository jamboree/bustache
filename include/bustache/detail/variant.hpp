/*//////////////////////////////////////////////////////////////////////////////
    Copyright (c) 2016 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////*/
#ifndef BUSTACHE_DETAIL_VARIANT_HPP_INCLUDED
#define BUSTACHE_DETAIL_VARIANT_HPP_INCLUDED

#include <cassert>
#include <cstdlib>
#include <utility>
#include <type_traits>

namespace bustache { namespace detail
{
    template<class T>
    inline T& cast(void* data)
    {
        return *static_cast<T*>(data);
    }

    template<class T>
    inline T const& cast(void const* data)
    {
        return *static_cast<T const*>(data);
    }

    [[noreturn]] inline void unreachable() { assert(!"cannot be here"); std::abort(); }

    struct ctor_visitor
    {
        using result_type = void;

        void* data;

        template<class T>
        void operator()(T& t) const
        {
            new(data) T(std::move(t));
        }

        template<class T>
        void operator()(T const& t) const
        {
            new(data) T(t);
        }
    };

    struct dtor_visitor
    {
        using result_type = void;

        template<class T>
        void operator()(T& t) const
        {
            t.~T();
        }
    };

    //template<class T>
    //struct get_visitor
    //{
    //    T* operator()(T& t) const
    //    {
    //        return &t;
    //    }

    //    template<class U>
    //    T* operator()(U&) const
    //    {
    //        return nullptr;
    //    }
    //};

    template<class T>
    struct variant {};

    template<class View>
    struct variant_ptr
    {
        variant_ptr() noexcept : _data() {}

        variant_ptr(std::nullptr_t) noexcept : _data() {}

        variant_ptr(unsigned which, void const* data) noexcept : _which(which), _data(data) {}

        explicit operator bool() const
        {
            return !!_data;
        }

        View operator*() const
        {
            return {_which, _data};
        }

    private:

        unsigned _which;
        void const* _data;
    };

    template<class T>
    struct type {};
}}

namespace bustache
{
    template<class Var, class Visitor>
    inline decltype(auto) apply_visitor(Visitor&& v, detail::variant<Var>& var)
    {
        return static_cast<Var&>(var).apply_visitor(v);
    }

    template<class Var, class Visitor>
    inline decltype(auto) apply_visitor(Visitor&& v, detail::variant<Var> const& var)
    {
        return static_cast<Var const&>(var).apply_visitor(v);
    }

    template<class T, class Var>
    inline T* get(detail::variant<Var>* var)
    {
        return var ? static_cast<Var*>(var)->template get<T>() : nullptr;
    }

    template<class T, class Var>
    inline T const* get(detail::variant<Var> const* var)
    {
        return var ? static_cast<Var const*>(var)->template get<T>() : nullptr;
    }

    template<class T, class Var>
    inline T const* get(detail::variant_ptr<Var> const& var)
    {
        return var ? (*var).template get<T>() : nullptr;
    }
}

#define Zz_BUSTACHE_VARIANT_SWITCH(N, U, D) case N: return v(detail::cast<U>(data));
#define Zz_BUSTACHE_VARIANT_MEMBER(N, U, D) U _##N;
#define Zz_BUSTACHE_VARIANT_CTOR(N, U, D)                                       \
D(U val) noexcept : _which(N), _##N(std::move(val)) {}
/***/
#define Zz_BUSTACHE_VARIANT_IS(N, U, D)                                         \
static bool is(unsigned which, detail::type<U>)                                 \
{                                                                               \
    return which == N;                                                          \
}                                                                               \
/***/
#define Zz_BUSTACHE_VARIANT_DECL(VAR, TYPES, NOEXCPET)                          \
VAR(VAR&& other) noexcept(NOEXCPET) : _which(other._which)                      \
{                                                                               \
    detail::ctor_visitor v{_storage};                                           \
    other.apply_visitor(v);                                                     \
}                                                                               \
VAR(VAR const& other) : _which(other._which)                                    \
{                                                                               \
    detail::ctor_visitor v{_storage};                                           \
    other.apply_visitor(v);                                                     \
}                                                                               \
~VAR()                                                                          \
{                                                                               \
    detail::dtor_visitor v;                                                     \
    apply_visitor(v);                                                           \
}                                                                               \
template<class T = VAR,                                                         \
    std::enable_if_t<std::is_constructible<VAR, T>::value, bool> = true>        \
VAR& operator=(T&& other) noexcept(noexcept(VAR(std::forward<T>(other))))       \
{                                                                               \
    this->~VAR();                                                               \
    return *new(this) VAR(std::forward<T>(other));                              \
}                                                                               \
template<class Visitor>                                                         \
typename Visitor::result_type apply_visitor(Visitor& v)                         \
{                                                                               \
    return switcher::visit(_which, data(), v);                                  \
}                                                                               \
template<class Visitor>                                                         \
typename Visitor::result_type apply_visitor(Visitor& v) const                   \
{                                                                               \
    return switcher::visit(_which, data(), v);                                  \
}                                                                               \
unsigned which() const                                                          \
{                                                                               \
    return _which;                                                              \
}                                                                               \
void* data()                                                                    \
{                                                                               \
    return _storage;                                                            \
}                                                                               \
void const* data() const                                                        \
{                                                                               \
    return _storage;                                                            \
}                                                                               \
template<class T>                                                               \
T* get()                                                                        \
{                                                                               \
    return switcher::is(_which, detail::type<T>{}) ?                            \
        static_cast<T*>(data()) : nullptr;                                      \
}                                                                               \
template<class T>                                                               \
T const* get() const                                                            \
{                                                                               \
    return switcher::is(_which, detail::type<T>{}) ?                            \
        static_cast<T const*>(data()) : nullptr;                                \
}                                                                               \
private:                                                                        \
struct switcher                                                                 \
{                                                                               \
    template<class T, class Visitor>                                            \
    static decltype(auto) visit(unsigned which, T* data, Visitor& v)            \
    {                                                                           \
        switch (which)                                                          \
        {                                                                       \
        TYPES(Zz_BUSTACHE_VARIANT_SWITCH,)                                      \
        default: detail::unreachable();                                         \
        }                                                                       \
    }                                                                           \
    TYPES(Zz_BUSTACHE_VARIANT_IS,)                                              \
};                                                                              \
unsigned _which;                                                                \
union                                                                           \
{                                                                               \
    char _storage[1];                                                           \
    TYPES(Zz_BUSTACHE_VARIANT_MEMBER,)                                          \
};                                                                              \
/***/

#endif