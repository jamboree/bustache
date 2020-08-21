/*//////////////////////////////////////////////////////////////////////////////
    Copyright (c) 2014-2020 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////*/
#ifndef BUSTACHE_MODEL_HPP_INCLUDED
#define BUSTACHE_MODEL_HPP_INCLUDED

#include <bustache/format.hpp>
#include <vector>
#include <concepts>
#include <functional>
#include <charconv>

namespace std
{
    template<class... T>
    class variant;
};

namespace bustache::detail
{
    struct vtable_base;

    template<class T, class Storage>
    constexpr bool can_store_in_v =
        sizeof(T) <= sizeof(Storage) &&
        alignof(T) <= alignof(Storage);

    template<class T>
    constexpr bool is_inplace_v =
        std::is_trivial_v<T> &&
        can_store_in_v<T, std::uintptr_t>;

    template<class T>
    inline decltype(auto) deref_data(std::uintptr_t self)
    {
        if constexpr (is_inplace_v<T>)
        {
            T ret;
            std::memcpy(&ret, &self, sizeof(T));
            return ret;
        }
        else
            return *reinterpret_cast<T const*>(self);
    }

    template<class T>
    inline std::uintptr_t encode_data(T const* p)
    {
        if constexpr (is_inplace_v<T>)
        {
            std::uintptr_t ret;
            std::memcpy(&ret, p, sizeof(T));
            return ret;
        }
        else
            return reinterpret_cast<std::uintptr_t>(p);
    }

    template<class T>
    struct range_value;

    template<class T, std::size_t N>
    struct range_value<T[N]>
    {
        using type = T;
    };

    template<class T> requires requires { typename T::value_type; }
    struct range_value<T>
    {
        using type = typename T::value_type;
    };

    template<class T>
    using range_value_t = typename range_value<T>::type;

    namespace barrier
    {
        using std::begin;
        using std::end;

        struct begin_fn
        {
            template<class T>
            auto operator()(T const& seq) const -> decltype(begin(seq))
            {
                return begin(seq);
            }
        };

        struct end_fn
        {
            template<class T>
            auto operator()(T const& seq) const -> decltype(end(seq))
            {
                return end(seq);
            }
        };
    }

    constexpr barrier::begin_fn begin{};
    constexpr barrier::end_fn end{};

    template<class>
    struct fn_base;

    template<class R, class... T>
    struct fn_base<R(T...)>
    {
        fn_base() noexcept : _data(), _call() {}

        template<class F>
        fn_base(F const& f) noexcept : _data(encode_data(&f)), _call(call<F>) {}

        template<class F>
        fn_base(F* f) noexcept : _data(reinterpret_cast<std::uintptr_t>(f)), _call(call<F*>) {}

        R operator()(T... t) const
        {
            return _call(_data, std::forward<T>(t)...);
        }

        template<class F>
        static R call(std::uintptr_t f, T&&... t)
        {
            return deref_data<F>(f)(std::forward<T>(t)...);
        }

        std::uintptr_t _data;
        R(*_call)(std::uintptr_t, T&&...);
    };

    struct content_visitor;
    struct object_ptr;
}

namespace bustache
{
    enum class model
    {
        null,
        atom,
        object,
        list,
        lazy_value,
        lazy_format
    };

    template<class T>
    struct impl_test;

    template<class T>
    struct impl_print;

    template<class T>
    struct impl_object;

    template<class T>
    struct impl_list;

    template<class T>
    struct impl_model;

    template<class T>
    struct impl_compatible;

    struct value_ptr;

    template<class T>
    concept Model = requires
    {
        {impl_model<T>::kind} -> std::convertible_to<model>;
    };

    template<class T>
    concept Compatible = requires(T const* p)
    {
        {impl_compatible<T>::get_value_ptr(*p)} -> std::same_as<value_ptr>;
    };

    template<class F>
    concept Lazy_format = requires(F const& f, ast::view const* view)
    {
        {f(view)} -> std::convertible_to<format>;
    };

    template<class T>
    concept Value = requires(T const& val)
    {
        value_ptr(&val);
    };

    template<class F>
    concept Lazy_value = requires(F const& f, ast::view const* view)
    {
        {f(view)} -> Value;
    };

    template<class T>
    concept Arithmetic = std::is_arithmetic_v<T>;

    template<class T>
    concept String = std::convertible_to<T, std::string_view> && !std::same_as<T, std::nullptr_t>;

    template<class T>
    concept ValueRange = requires(T const& t)
    {
        detail::begin(t);
        detail::end(t);
    } && Value<detail::range_value_t<T>>;

    template<class T>
    concept StrValueMap = requires(T const& t, std::string const& key)
    {
        t.find(key) == t.end();
    } && Value<typename T::mapped_type>;

    template<class F, class R, class... T>
    concept Callable = requires(F const& f, T... t)
    {
        {f(t...)} -> std::convertible_to<R>;
    };

    template<class>
    class fn_ref;

    template<class R, class... T>
    class fn_ref<R(T...)> : detail::fn_base<R(T...)>
    {
        using base_t = detail::fn_base<R(T...)>;

    public:
        template<Callable<R, T...> F>
        fn_ref(F const& f) noexcept : base_t(f) {}

        using base_t::operator();
    };

    template<class>
    class fn_ptr;

    template<class R, class... T>
    class fn_ptr<R(T...)> : detail::fn_base<R(T...)>
    {
        using base_t = detail::fn_base<R(T...)>;

    public:
        fn_ptr() = default;
        fn_ptr(std::nullptr_t) noexcept {}

        template<Callable<R, T...> F>
        fn_ptr(F const& f) noexcept : base_t(f) {}

        using base_t::operator();

        explicit operator bool() const { return !!this->_data; }
    };

    using output_handler = fn_ref<void(char const*, std::size_t)>;

    struct value_ptr
    {
        constexpr value_ptr() { reset(); }
        constexpr value_ptr(std::nullptr_t) { reset(); }

        template<Model T>
        value_ptr(T const* p) noexcept { p ? init_model(p) : reset(); }

        template<Compatible T>
        value_ptr(T const* p) noexcept { p ? init_compatible(p) : reset(); }

        template<Lazy_value F>
        value_ptr(F const* f) noexcept { f ? init_lazy_value(f) : reset(); }

        template<Lazy_format F>
        value_ptr(F const* f) noexcept { f ? init_lazy_format(f) : reset(); }

        explicit operator bool() const noexcept;

        constexpr void reset();

    private:
        template<class T>
        void init_model(T const* p) noexcept;

        template<class F>
        void init_lazy_format(F const* f) noexcept;

        template<class F>
        void init_lazy_value(F const* f) noexcept;

        template<class T>
        void init_compatible(T const* p) noexcept
        {
            *this = impl_compatible<T>::get_value_ptr(*p);
        }

        friend struct detail::content_visitor;
        friend struct detail::object_ptr;

        std::uintptr_t data;
        detail::vtable_base const* vptr;
    };

    using value_handler = fn_ref<void(value_ptr)>;
}

namespace bustache::detail
{
    template<class T>
    struct type {};

    struct vtable_base
    {
        model kind;
    };

    struct lazy_format_vtable : vtable_base
    {
        template<class F>
        constexpr lazy_format_vtable(type<F> t) : vtable_base{model::lazy_format}, call(call_impl<F>) {}

        format(*call)(std::uintptr_t, ast::view const*);

        template<class F>
        static format call_impl(std::uintptr_t self, ast::view const* view)
        {
            return deref_data<F>(self)(view);
        }
    };

    template<class F>
    constexpr lazy_format_vtable lazy_format_vt{type<F>{}};

    struct lazy_value_vtable : vtable_base
    {
        template<class F>
        constexpr lazy_value_vtable(type<F> t) : vtable_base{model::lazy_value}, call(call_impl<F>) {}

        void(*call)(std::uintptr_t, ast::view const*, value_handler visit);

        template<class F>
        static void call_impl(std::uintptr_t self, ast::view const* view, value_handler visit)
        {
            auto const& val = deref_data<F>(self)(view);
            visit(&val);
        }
    };

    template<class F>
    constexpr lazy_value_vtable lazy_value_vt{type<F>{}};

    struct test_trait
    {
        constexpr test_trait(...) : test(test_default<true>) {}

        constexpr test_trait(type<void>) : test(test_default<false>) {}

        template<class T> requires requires{impl_test<T>{};}
        constexpr test_trait(type<T>) : test(test_impl<T>) {}

        bool(*test)(std::uintptr_t self);

        template<bool Value>
        static bool test_default(std::uintptr_t)
        {
            return Value;
        }

        template<class T>
        static bool test_impl(std::uintptr_t self)
        {
            return impl_test<T>::test(deref_data<T>(self));
        }
    };

    struct print_trait
    {
        constexpr print_trait(...) : print(print_default) {}

        template<class T> requires requires{impl_print<T>{};}
        constexpr print_trait(type<T>) : print(print_impl<T>) {}

        void(*print)(std::uintptr_t self, output_handler os, char const* fmt);

        static void print_default(std::uintptr_t, output_handler, char const*) {}

        template<class T>
        static void print_impl(std::uintptr_t self, output_handler os, char const* fmt)
        {
            return impl_print<T>::print(deref_data<T>(self), os, fmt);
        }
    };

    struct object_trait
    {
        constexpr object_trait(...) : get(get_default) {}

        template<class T> requires requires{impl_object<T>{};}
        constexpr object_trait(type<T>) : get(get_impl<T>) {}

        void(*get)(std::uintptr_t self, std::string const& key, value_handler visit);

        static void get_default(std::uintptr_t, std::string const&, value_handler visit)
        {
            visit(nullptr);
        }

        template<class T>
        static void get_impl(std::uintptr_t self, std::string const& key, value_handler visit)
        {
            return impl_object<T>::get(deref_data<T>(self), key, visit);
        }
    };

    struct list_trait
    {
        constexpr list_trait(...) : iterate() {}

        template<class T> requires requires{impl_list<T>{};}
        constexpr list_trait(type<T>) : iterate(iterate_impl<T>) {}

        void(*iterate)(std::uintptr_t self, value_handler visit);

        template<class T>
        static void iterate_impl(std::uintptr_t self, value_handler visit)
        {
            return impl_list<T>::iterate(deref_data<T>(self), visit);
        }
    };

    struct value_vtable : vtable_base, test_trait, print_trait, object_trait, list_trait
    {
        constexpr value_vtable(type<void> t)
            : vtable_base{model::null}
            , test_trait(t), print_trait(t), object_trait(t), list_trait(t)
        {}

        template<class T>
        constexpr value_vtable(type<T> t)
            : vtable_base{impl_model<T>::kind}
            , test_trait(t), print_trait(t), object_trait(t), list_trait(t)
        {}
    };

    template<class T>
    constexpr value_vtable value_vt{type<T>{}};

    template<model K, class T>
    struct check_model;

    template<class T>
    struct check_model<model::atom, T> : impl_test<T>, impl_print<T> {};

    template<class T>
    struct check_model<model::object, T> : impl_object<T> {};

    template<class T>
    struct check_model<model::list, T> : impl_list<T> {};
}

namespace bustache
{
    inline value_ptr::operator bool() const noexcept
    {
        return vptr->kind != model::null;
    }

    constexpr void value_ptr::reset()
    {
        data = 0;
        vptr = &detail::value_vt<void>;
    }

    template<class T>
    inline void value_ptr::init_model(T const* p) noexcept
    {
        sizeof(detail::check_model<impl_model<T>::kind, T>);
        data = detail::encode_data(p);
        vptr = &detail::value_vt<T>;
    }

    template<class F>
    inline void value_ptr::init_lazy_format(F const* f) noexcept
    {
        data = detail::encode_data(f);
        vptr = &detail::lazy_format_vt<F>;
    }

    template<class F>
    inline void value_ptr::init_lazy_value(F const* f) noexcept
    {
        data = detail::encode_data(f);
        vptr = &detail::lazy_value_vt<F>;
    }

    template<>
    struct impl_model<bool>
    {
        static constexpr model kind = model::atom;
    };

    template<>
    struct impl_test<bool>
    {
        static bool test(bool self)
        {
            return self;
        }
    };

    template<>
    struct impl_print<bool>
    {
        static void print(bool self, output_handler os, char const* fmt)
        {
            self ? os("true", 4) : os("false", 5);
        }
    };

    template<Arithmetic T>
    struct impl_model<T>
    {
        static constexpr model kind = model::atom;
    };

    template<Arithmetic T>
    struct impl_test<T>
    {
        static bool test(T self)
        {
            return !!self;
        }
    };

    template<Arithmetic T>
    struct impl_print<T>
    {
        static void print(T self, output_handler os, char const* fmt)
        {
            char buf[256];
            auto const [p, e] = std::to_chars(std::begin(buf), std::end(buf), self);
            if (e == std::errc())
                os(buf, p - buf);
        }
    };

    template<String T>
    struct impl_model<T>
    {
        static constexpr model kind = model::atom;
    };
    
    template<String T>
    struct impl_test<T>
    {
        static bool test(std::string_view self)
        {
            return !self.empty();
        }
    };

    template<String T>
    struct impl_print<T>
    {
        static void print(std::string_view self, output_handler os, char const* fmt)
        {
            os(self.data(), self.size());
        }
    };

    template<StrValueMap T>
    struct impl_model<T>
    {
        static constexpr model kind = model::object;
    };

    template<class T> requires (impl_model<T>::kind == model::object)
    struct impl_test<T>; // Intentionally undefined.

    template<StrValueMap T>
    struct impl_object<T>
    {
        static void get(T const& self, std::string const& key, value_handler visit)
        {
            auto const found = self.find(key);
            visit(found == self.end() ? nullptr : &found->second);
        }
    };

    template<ValueRange T> requires !String<T> && !StrValueMap<T>
    struct impl_model<T>
    {
        static constexpr model kind = model::list;
    };

    template<class T> requires (impl_model<T>::kind == model::list)
    struct impl_test<T>
    {
        static bool test(T const& self)
        {
            return !impl_list<T>::empty(self);
        }
    };

    template<ValueRange T>
    struct impl_list<T>
    {
        static bool empty(T const& self)
        {
            return std::empty(self);
        }

        static void iterate(T const& self, value_handler visit)
        {
            for (auto const& elem : self)
                visit(&elem);
        }
    };

    template<String K, Value V>
    struct impl_model<std::pair<K, V>>
    {
        static constexpr model kind = model::object;
    };

    template<String K, Value V>
    struct impl_object<std::pair<K, V>>
    {
        static void get(std::pair<K, V> const& self, std::string const& key, value_handler visit)
        {
            if (key == "key")
                return visit(&self.first);
            if (key == "value")
                return visit(&self.second);
            return visit(nullptr);
        }
    };

    template<>
    struct impl_compatible<std::nullptr_t>
    {
        static value_ptr get_value_ptr(std::nullptr_t)
        {
            return nullptr;
        }
    };

    template<Value... T>
    struct impl_compatible<std::variant<T...>>
    {
        static value_ptr get_value_ptr(std::variant<T...> const& self)
        {
            return visit([](auto const& val) { return value_ptr(&val); }, self);
        }
    };
}

#endif