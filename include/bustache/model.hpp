/*//////////////////////////////////////////////////////////////////////////////
    Copyright (c) 2014-2018 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////*/
#ifndef BUSTACHE_MODEL_HPP_INCLUDED
#define BUSTACHE_MODEL_HPP_INCLUDED

#include <bustache/format.hpp>
#include <bustache/detail/variant.hpp>
#include <bustache/detail/any_context.hpp>
#include <vector>
#include <functional>
#include <boost/unordered_map.hpp>

namespace bustache
{
    class value;

    using array = std::vector<value>;

    // We use boost::unordered_map because it allows incomplete type.
    using object = boost::unordered_map<std::string, value>;

    using lambda0v = std::function<value()>;

    using lambda0f = std::function<format()>;

    using lambda1v = std::function<value(ast::content_list const&)>;

    using lambda1f = std::function<format(ast::content_list const&)>;

    struct value_view;

    using value_ptr = variant_ptr<value_view>;

    struct value_holder;

    template<class T>
    struct object_trait;

    // SYNOPSIS
    // --------
    // template<>
    // struct object_trait<T>
    // {
    //     static value_ptr get(T const& self, std::string const& key, value_holder& hold);
    // };

    struct object_view
    {
        object_view() noexcept : _data(), _get(&empty_get) {}

        template<class T>
        explicit object_view(T const& obj) noexcept
          : _data(&obj), _get(&get_impl<T>)
        {}

        object_view(object const& obj) noexcept;

        explicit operator bool() const noexcept
        {
            return !!_data;
        }

        value_ptr get(std::string const& key, value_holder& hold) const
        {
            return _get(_data, key, hold);
        }

    private:
        static value_ptr empty_get(void const*, std::string const&, value_holder&)
        {
            return {};
        }

        template<class T>
        static value_ptr get_impl(void const* p, std::string const& key, value_holder& hold)
        {
            return object_trait<T>::get(*static_cast<T const*>(p), key, hold);
        }

        void const* _data;
        value_ptr(*_get)(void const*, std::string const&, value_holder&);
    };

    template<class T>
    struct list_trait;

    // SYNOPSIS
    // --------
    // template<>
    // struct list_trait<T>
    // {
    //     static bool empty(T const& self);
    //     static std::uintptr_t begin_cursor(T const& self);
    //     static value_ptr next(T const& self, std::uintptr_t& state, value_holder& hold);
    //     static void end_cursor(T const& self, std::uintptr_t) noexcept {}
    // };

    struct list_view
    {
        template<class T>
        explicit list_view(T const& arr) noexcept
          : _vptr(&impl<T>::table), _data(&arr)
        {}

        list_view(array const& arr) noexcept;

        bool empty() const
        {
            return _vptr->_empty(_data);
        }

        std::uintptr_t begin_cursor() const
        {
            return _vptr->_begin_cursor(_data);
        }

        value_ptr next(std::uintptr_t& state, value_holder& hold) const
        {
            return _vptr->_next(_data, state, hold);
        }

        void end_cursor(std::uintptr_t state) noexcept
        {
            _vptr->_end_cursor(_data, state);
        }

    private:
        struct vtable
        {
            value_ptr(*_next)(void const*, std::uintptr_t&, value_holder&);
            std::uintptr_t(*_begin_cursor)(void const*);
            void(*_end_cursor)(void const*, std::uintptr_t) noexcept;
            bool(*_empty)(void const*);
        };

        template<class T>
        struct impl
        {
            static value_ptr next(void const* p, std::uintptr_t& state, value_holder& hold)
            {
                return list_trait<T>::next(*static_cast<T const*>(p), state, hold);
            }

            static std::uintptr_t begin_cursor(void const* p)
            {
                return list_trait<T>::begin_cursor(*static_cast<T const*>(p));
            }

            static void end_cursor(void const* p, std::uintptr_t state) noexcept
            {
                list_trait<T>::end_cursor(*static_cast<T const*>(p), state);
            }

            static bool empty(void const* p)
            {
                return list_trait<T>::empty(*static_cast<T const*>(p));
            }

            static constexpr vtable table = {&next, &begin_cursor, &end_cursor, &empty};
        };

        vtable const* _vptr;
        void const* _data;
    };

#define BUSTACHE_VALUE(X, D)                                                    \
    X(0, std::nullptr_t, D)                                                     \
    X(1, bool, D)                                                               \
    X(2, int, D)                                                                \
    X(3, double, D)                                                             \
    X(4, std::string, D)                                                        \
    X(5, array, D)                                                              \
    X(6, lambda0v, D)                                                           \
    X(7, lambda0f, D)                                                           \
    X(8, lambda1v, D)                                                           \
    X(9, lambda1f, D)                                                           \
    X(10, object, D)                                                            \
    X(11, object_view, D)                                                       \
    X(12, list_view, D)                                                         \
/***/

    namespace detail
    {
        struct value_type_matcher
        {
            static std::nullptr_t match(std::nullptr_t);
            static int match(int);
            // Prevent unintended bool conversion.
            template<class Bool, typename std::enable_if<std::is_same<Bool, bool>::value, bool>::type = true>
            static bool match(Bool);
            static double match(double);
            static std::string match(std::string);
            // Need to override for `char const*`, otherwise `bool` will be chosen.
            static std::string match(char const*);
            static array match(array);
            static lambda0v match(lambda0v);
            static lambda0f match(lambda0f);
            static lambda1v match(lambda1v);
            static lambda1f match(lambda1f);
            static object match(object);
            static object_view match(object_view);
            static list_view match(list_view);
        };

        union value_union
        {
            BUSTACHE_VALUE(Zz_BUSTACHE_VARIANT_MEMBER, )
        };

        union holder_storage
        {
            void* ptr;
            alignas(std::max_align_t) char data[32];
        };

        template<class T, bool InPlace = sizeof(T) <= 32 && alignof(T) <= alignof(std::max_align_t)>
        struct holder_trait
        {
            static T& create(holder_storage& store, T&& val)
            {
                return *new(store.data) T(std::move(val));
            }

            template<class X>
            static void destroy(void* p) noexcept
            {
                p = static_cast<X*>(p)->store.data;
                static_cast<T*>(p)->~T();
            }
        };

        template<class T>
        struct holder_trait<T, false>
        {
            static T& create(holder_storage& store, T&& val)
            {
                auto p = new T(std::move(val));
                store.ptr = p;
                return *p;
            }

            template<class X>
            static void destroy(void* p) noexcept
            {
                p = static_cast<X*>(p)->store.ptr;
                delete static_cast<T*>(p);
            }
        };

        struct object_holder
        {
            object_view view;
            holder_storage store;
        };

        struct list_holder
        {
            list_view view;
            holder_storage store;
        };
    }

    class value : public variant_base<value>
    {
        using type_matcher = detail::value_type_matcher;
        unsigned _which;
        union
        {
            char _storage[1];
            detail::value_union _union;
        };
    public:

        friend struct value_view;
        using view = value_view;
        using pointer = value_ptr;

        Zz_BUSTACHE_VARIANT_DECL(value, BUSTACHE_VALUE, false)

        value() noexcept : _which(0) {}

        pointer get_pointer() const noexcept
        {
            return {_which, _storage};
        }
    };

    struct value_view : variant_base<value_view>
    {
        using switcher = value::switcher;

#define BUSTACHE_VALUE_VIEW_CTOR(N, U, D)                                       \
        value_view(U const& data) noexcept : _which(N), _data(&data) {}
        BUSTACHE_VALUE(BUSTACHE_VALUE_VIEW_CTOR,)
#undef BUSTACHE_VALUE_VIEW_CTOR

        value_view(value const& data) noexcept
          : _which(data._which), _data(data._storage)
        {}

        value_view(unsigned which, void const* data) noexcept
          : _which(which), _data(data)
        {}

        unsigned which() const noexcept
        {
            return _which;
        }

        void const* data() const noexcept
        {
            return _data;
        }

        value_ptr get_pointer() const noexcept
        {
            return {_which, _data};
        }

    private:

        unsigned _which;
        void const* _data;
    };
#undef BUSTACHE_VALUE

    struct value_holder
    {
        using type_matcher = detail::value_type_matcher;
        using switcher = value::switcher;

        value_holder() noexcept : _destroy() {}

        ~value_holder()
        {
            if (_destroy)
                _destroy(_storage);
        }

        bool used() const noexcept { return !!_destroy; }

        template<class T, class U = decltype(type_matcher::match(std::declval<T>()))>
        value_ptr operator()(T&& other)
        {
            auto p = new(_storage) U(std::forward<T>(other));
            _destroy = &destroy<U>;
            return {switcher::index(detail::type<U>{}), p};
        }

        template<class T>
        value_ptr object(T obj)
        {
            using trait = detail::holder_trait<T>;
            _obj.view = object_view(trait::create(_obj.store, std::move(obj)));
            _destroy = &trait::template destroy<detail::object_holder>;
            return {switcher::index(detail::type<object_view>{}), &_obj.view};
        }

        template<class T>
        value_ptr list(T lst)
        {
            using trait = detail::holder_trait<T>;
            _lst.view = list_view(trait::create(_lst.store, std::move(lst)));
            _destroy = &trait::template destroy<detail::list_holder>;
            return {switcher::index(detail::type<list_view>{}), &_lst.view};
        }

    private:
        template<class T>
        static void destroy(void* p) noexcept
        {
            static_cast<T*>(p)->~T();
        }

        void(*_destroy)(void*) noexcept;
        union
        {
            char _storage[1];
            detail::value_union _union;
            detail::object_holder _obj;
            detail::list_holder _lst;
        };
    };

    template<>
    struct object_trait<object>
    {
        static value_ptr get(object const& obj, std::string const& key, value_holder&)
        {
            auto it = obj.find(key);
            return it == obj.end() ? value_ptr() : it->second.get_pointer();
        }
    };

    inline object_view::object_view(object const& obj) noexcept
      : _data(&obj), _get(&get_impl<object>)
    {}

    inline object_view get_object(value_ptr p) noexcept
    {
        if (p)
        {
            auto const which = p.which();
            if (value::switcher::index(detail::type<object>{}) == which)
                return *static_cast<object const*>(p.data());
            if (value::switcher::index(detail::type<object_view>{}) == which)
                return *static_cast<object_view const*>(p.data());
        }
        return {};
    }
}

namespace bustache
{
    struct default_unresolved_handler
    {
        value operator()(std::string const& /*key*/) const
        {
            return nullptr;
        }
    };

    // Forward decl only.
    template<class CharT, class Traits, class Context, class UnresolvedHandler = default_unresolved_handler>
    void generate_ostream
    (
        std::basic_ostream<CharT, Traits>& out, format const& fmt,
        value_view const& data, Context const& context,
        option_type flag, UnresolvedHandler&& f = {}
    );

    // Forward decl only.
    template<class String, class Context, class UnresolvedHandler = default_unresolved_handler>
    void generate_string
    (
        String& out, format const& fmt,
        value_view const& data, Context const& context,
        option_type flag, UnresolvedHandler&& f = {}
    );

    template<class CharT, class Traits, class T, class Context,
        typename std::enable_if<std::is_constructible<value_view, T>::value, bool>::type = true>
    inline std::basic_ostream<CharT, Traits>&
    operator<<(std::basic_ostream<CharT, Traits>& out, manipulator<T, Context> const& manip)
    {
        generate_ostream(out, manip.fmt, manip.data, detail::any_context(manip.context), manip.flag);
        return out;
    }

    template<class T, class Context,
        typename std::enable_if<std::is_constructible<value_view, T>::value, bool>::type = true>
    inline std::string to_string(manipulator<T, Context> const& manip)
    {
        std::string ret;
        generate_string(ret, manip.fmt, manip.data, detail::any_context(manip.context), manip.flag);
        return ret;
    }
}

#endif