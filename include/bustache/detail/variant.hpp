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

    struct assign_visitor
    {
        using result_type = void;

        void* data;

        template<class T>
        void operator()(T& t) const
        {
            *static_cast<T*>(data) = std::move(t);
        }

        template<class T>
        void operator()(T const& t) const
        {
            *static_cast<T*>(data) = t;
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


#define Zz_BUSTACHE_UNREACHABLE(MSG) { assert(!MSG); std::abort(); }
#define Zz_BUSTACHE_VARIANT_SWITCH(N, U, D) case N: return v(detail::cast<U>(data));
#define Zz_BUSTACHE_VARIANT_MEMBER(N, U, D) U _##N;
#define Zz_BUSTACHE_VARIANT_CTOR(N, U, D)                                       \
D(U val) noexcept : _which(N), _##N(std::move(val)) {}
/***/
#define Zz_BUSTACHE_VARIANT_INDEX(N, U, D)                                      \
static constexpr unsigned index(detail::type<U>) { return N; }                  \
/***/
#define Zz_BUSTACHE_VARIANT_MATCH(N, U, D) static U match_type(U);
#define Zz_BUSTACHE_VARIANT_DECL(VAR, TYPES, NOEXCPET)                          \
private:                                                                        \
struct switcher                                                                 \
{                                                                               \
    template<class T, class Visitor>                                            \
    static decltype(auto) visit(unsigned which, T* data, Visitor& v)            \
    {                                                                           \
        switch (which)                                                          \
        {                                                                       \
        TYPES(Zz_BUSTACHE_VARIANT_SWITCH,)                                      \
        default: Zz_BUSTACHE_UNREACHABLE("cannot be here");                     \
        }                                                                       \
    }                                                                           \
    TYPES(Zz_BUSTACHE_VARIANT_INDEX,)                                           \
};                                                                              \
TYPES(Zz_BUSTACHE_VARIANT_MATCH,)                                               \
unsigned _which;                                                                \
union                                                                           \
{                                                                               \
    char _storage[1];                                                           \
    TYPES(Zz_BUSTACHE_VARIANT_MEMBER,)                                          \
};                                                                              \
void invalidate()                                                               \
{                                                                               \
    if (valid())                                                                \
    {                                                                           \
        detail::dtor_visitor v;                                                 \
        switcher::visit(_which, data(), v);                                     \
        _which = ~0u;                                                           \
    }                                                                           \
}                                                                               \
template<class T>                                                               \
void do_init(T& other)                                                          \
{                                                                               \
    detail::ctor_visitor v{_storage};                                           \
    switcher::visit(other._which, other.data(), v);                             \
}                                                                               \
template<class T>                                                               \
void do_assign(T& other)                                                        \
{                                                                               \
    if (_which == other._which)                                                 \
    {                                                                           \
        detail::assign_visitor v{_storage};                                     \
        switcher::visit(other._which, other.data(), v);                         \
    }                                                                           \
    else                                                                        \
    {                                                                           \
        invalidate();                                                           \
        if (other.valid())                                                      \
        {                                                                       \
            do_init(other);                                                     \
            _which = other._which;                                              \
        }                                                                       \
    }                                                                           \
}                                                                               \
public:                                                                         \
unsigned which() const                                                          \
{                                                                               \
    return _which;                                                              \
}                                                                               \
bool valid() const                                                              \
{                                                                               \
    return _which != ~0u;                                                       \
}                                                                               \
void* data()                                                                    \
{                                                                               \
    return _storage;                                                            \
}                                                                               \
void const* data() const                                                        \
{                                                                               \
    return _storage;                                                            \
}                                                                               \
template<class Visitor>                                                         \
decltype(auto) apply_visitor(Visitor& v)                                        \
{                                                                               \
    return switcher::visit(_which, data(), v);                                  \
}                                                                               \
template<class Visitor>                                                         \
decltype(auto) apply_visitor(Visitor& v) const                                  \
{                                                                               \
    return switcher::visit(_which, data(), v);                                  \
}                                                                               \
template<class T>                                                               \
T* get()                                                                        \
{                                                                               \
    return switcher::index(detail::type<T>{}) == _which ?                       \
        static_cast<T*>(data()) : nullptr;                                      \
}                                                                               \
template<class T>                                                               \
T const* get() const                                                            \
{                                                                               \
    return switcher::index(detail::type<T>{}) == _which ?                       \
        static_cast<T const*>(data()) : nullptr;                                \
}                                                                               \
VAR(VAR&& other) noexcept(NOEXCPET) : _which(other._which)                      \
{                                                                               \
    do_init(other);                                                             \
}                                                                               \
VAR(VAR const& other) : _which(other._which)                                    \
{                                                                               \
    do_init(other);                                                             \
}                                                                               \
template<class T, class U = decltype(match_type(std::declval<T>()))>            \
VAR(T&& other) noexcept(std::is_nothrow_constructible<U, T>::value)             \
  : _which(switcher::index(detail::type<U>{}))                                  \
{                                                                               \
    new(_storage) U(std::forward<T>(other));                                    \
}                                                                               \
~VAR()                                                                          \
{                                                                               \
    if (valid())                                                                \
    {                                                                           \
        detail::dtor_visitor v;                                                 \
        switcher::visit(_which, data(), v);                                     \
    }                                                                           \
}                                                                               \
template<class T, class U = decltype(match_type(std::declval<T>()))>            \
U& operator=(T&& other) noexcept(std::is_nothrow_constructible<U, T>::value)    \
{                                                                               \
    if (auto p = get<U>())                                                      \
        return *p = std::forward<T>(other);                                     \
    else                                                                        \
    {                                                                           \
        invalidate();                                                           \
        p = new(_storage) U(std::forward<T>(other));                            \
        _which = switcher::index(detail::type<U>{});                            \
        return *p;                                                              \
    }                                                                           \
}                                                                               \
VAR& operator=(VAR&& other) noexcept(NOEXCPET)                                  \
{                                                                               \
    do_assign(other);                                                           \
    return *this;                                                               \
}                                                                               \
VAR& operator=(VAR const& other)                                                \
{                                                                               \
    do_assign(other);                                                           \
    return *this;                                                               \
}                                                                               \
/***/

#endif