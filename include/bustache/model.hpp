/*//////////////////////////////////////////////////////////////////////////////
    Copyright (c) 2014-2016 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////*/
#ifndef BUSTACHE_MODEL_HPP_INCLUDED
#define BUSTACHE_MODEL_HPP_INCLUDED

#include "format.hpp"
#include "detail/variant.hpp"
#include <vector>
#include <cstdio> // for snprintf
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
/***/

    class value : public detail::variant<value>
    {
        // Need to override for `char const*`, otherwise `bool` will be chosen
        static std::string match_type(char const*);

    public:

        struct view;
        using pointer = detail::variant_ptr<view>;

        Zz_BUSTACHE_VARIANT_DECL(value, BUSTACHE_VALUE, false)

        value() noexcept : _which(0), _0() {}

        pointer get_pointer() const
        {
            return {_which, _storage};
        }
    };

    struct value::view : detail::variant<view>
    {
#define BUSTACHE_VALUE_VIEW_CTOR(N, U, D)                                       \
        view(U const& data) noexcept : _which(N), _data(&data) {}
        BUSTACHE_VALUE(BUSTACHE_VALUE_VIEW_CTOR,)
#undef BUSTACHE_VALUE_VIEW_CTOR

        view(value const& data) noexcept
          : _which(data._which), _data(data._storage)
        {}

        view(unsigned which, void const* data) noexcept
          : _which(which), _data(data)
        {}

        template<class Visitor>
        decltype(auto) apply_visitor(Visitor& v) const
        {
            return switcher::visit(_which, _data, v);
        }

        template<class T>
        T const* get() const
        {
            return switcher::index(detail::type<T>{}) == _which ?
                static_cast<T const*>(_data) : nullptr;
        }

        pointer get_pointer() const
        {
            return {_which, _data};
        }

    private:

        unsigned _which;
        void const* _data;
    };
#undef BUSTACHE_VALUE
}

namespace bustache { namespace detail
{
    inline value::pointer find(object const& data, std::string const& key)
    {
        auto it = data.find(key);
        if (it != data.end())
            return it->second.get_pointer();
        return nullptr;
    }

    template<class Sink>
    struct value_printer
    {
        typedef void result_type;
        
        Sink const& sink;
        bool const escaping;

        void operator()(std::nullptr_t) const {}
        
        template<class T>
        void operator()(T data) const
        {
            sink(data);
        }

        void operator()(std::string const& data) const
        {
            auto it = data.data(), end = it + data.size();
            if (escaping)
                escape_html(it, end);
            else
                sink(it, end);
        }
        
        void operator()(array const& data) const
        {
            auto it = data.begin(), end = data.end();
            if (it != end)
            {
                apply_visitor(*this, *it);
                while (++it != end)
                {
                    literal(",");
                    apply_visitor(*this, *it);
                }
            }
        }

        void operator()(object const&) const
        {
            literal("[Object]");
        }

        void operator()(lambda0v const& data) const
        {
            apply_visitor(*this, data());
        }

        void operator()(lambda1v const& data) const
        {
            apply_visitor(*this, data({}));
        }

        template<class Sig>
        void operator()(std::function<Sig> const&) const
        {
            literal("[Function]");
        }

        void escape_html(char const* it, char const* end) const
        {
            char const* last = it;
            while (it != end)
            {
                switch (*it)
                {
                case '&': sink(last, it); literal("&amp;"); break;
                case '<': sink(last, it); literal("&lt;"); break;
                case '>': sink(last, it); literal("&gt;"); break;
                case '\\': sink(last, it); literal("&#92;"); break;
                case '"': sink(last, it); literal("&quot;"); break;
                default:  ++it; continue;
                }
                last = ++it;
            }
            sink(last, it);
        }

        template<std::size_t N>
        void literal(char const (&str)[N]) const
        {
            sink(str, str + (N - 1));
        }
    };

    struct content_scope
    {
        content_scope const* const parent;
        object const& data;

        value::pointer lookup(std::string const& key) const
        {
            if (auto pv = find(data, key))
                return pv;
            if (parent)
                return parent->lookup(key);
            return nullptr;
        }
    };

    struct content_visitor_base
    {
        using result_type = void;

        content_scope const* scope;
        value::pointer cursor;
        mutable std::string key_cache;

        value::pointer resolve(std::string const& key) const
        {
            auto ki = key.begin();
            auto ke = key.end();
            if (ki == ke)
                return {};
            value::pointer pv = nullptr;
            if (*ki == '.')
            {
                if (++ki == ke)
                    return cursor;
                auto k0 = ki;
                while (*ki != '.' && ++ki != ke);
                key_cache.assign(k0, ki);
                pv = find(scope->data, key_cache);
            }
            else
            {
                auto k0 = ki;
                while (ki != ke && *ki != '.') ++ki;
                key_cache.assign(k0, ki);
                pv = scope->lookup(key_cache);
            }
            if (ki == ke)
                return pv;
            if (auto obj = get<object>(pv))
            {
                auto k0 = ++ki;
                while (ki != ke)
                {
                    if (*ki == '.')
                    {
                        key_cache.assign(k0, ki);
                        obj = get<object>(find(*obj, key_cache));
                        if (!obj)
                            return nullptr;
                        k0 = ++ki;
                    }
                    else
                        ++ki;
                }
                key_cache.assign(k0, ki);
                return find(*obj, key_cache);
            }
            return nullptr;
        }
    };

    template<class ContentVisitor>
    struct variable_visitor : value_printer<typename ContentVisitor::sink_type>
    {
        using base_type = value_printer<typename ContentVisitor::sink_type>;
        
        ContentVisitor& parent;

        variable_visitor(ContentVisitor& parent, bool escaping)
          : base_type{parent.sink, escaping}, parent(parent)
        {}

        using base_type::operator();

        void operator()(lambda0f const& data) const
        {
            auto fmt(data());
            for (auto const& content : fmt.contents())
                apply_visitor(parent, content);
        }
    };

    template<class ContentVisitor>
    struct section_visitor
    {
        using result_type = bool;

        ContentVisitor& parent;
        ast::content_list const& contents;
        bool const inverted;

        bool operator()(object const& data) const
        {
            if (!inverted)
            {
                content_scope scope{parent.scope, data};
                auto old_scope = parent.scope;
                parent.scope = &scope;
                for (auto const& content : contents)
                    apply_visitor(parent, content);
                parent.scope = old_scope;
            }
            return false;
        }

        bool operator()(array const& data) const
        {
            if (inverted)
                return data.empty();

            for (auto const& val : data)
            {
                parent.cursor = val.get_pointer();
                if (auto obj = get<object>(&val))
                {
                    content_scope scope{parent.scope, *obj};
                    auto old_scope = parent.scope;
                    parent.scope = &scope;
                    for (auto const& content : contents)
                        apply_visitor(parent, content);
                    parent.scope = old_scope;
                }
                else
                {
                    for (auto const& content : contents)
                        apply_visitor(parent, content);
                }
            }
            return false;
        }

        bool operator()(bool data) const
        {
            return data ^ inverted;
        }

        // The 2 overloads below are not necessary but to suppress
        // the stupid MSVC warning.
        bool operator()(int data) const
        {
            return !!data ^ inverted;
        }

        bool operator()(double data) const
        {
            return !!data ^ inverted;
        }

        bool operator()(std::string const& data) const
        {
            return !data.empty() ^ inverted;
        }

        bool operator()(std::nullptr_t) const
        {
            return inverted;
        }

        bool operator()(lambda0v const& data) const
        {
            return inverted ? false : apply_visitor(*this, data());
        }

        bool operator()(lambda0f const& data) const
        {
            if (!inverted)
            {
                auto fmt(data());
                for (auto const& content : fmt.contents())
                    apply_visitor(parent, content);
            }
            return false;
        }

        bool operator()(lambda1v const& data) const
        {
            return inverted ? false : apply_visitor(*this, data(contents));
        }

        bool operator()(lambda1f const& data) const
        {
            if (!inverted)
            {
                auto fmt(data(contents));
                for (auto const& content : fmt.contents())
                    apply_visitor(parent, content);
            }
            return false;
        }
    };

    template<class Sink, class Context>
    struct content_visitor : content_visitor_base
    {
        using sink_type = Sink;

        Sink const& sink;
        Context const& context;
        std::string indent;
        bool needs_indent;
        bool const escaping;

        content_visitor
        (
            content_scope const& scope, value::pointer cursor,
            Sink const &sink, Context const &context, bool escaping
        )
          : content_visitor_base{&scope, cursor, {}}
          , sink(sink), context(context), needs_indent(), escaping(escaping)
        {}

        void operator()(ast::text const& text)
        {
            auto i = text.begin();
            auto e = text.end();
            assert(i != e && "empty text shouldn't be in ast");
            if (indent.empty())
            {
                sink(i, e);
                return;
            }
            --e; // Don't flush indent on last newline.
            auto const ib = indent.data();
            auto const ie = ib + indent.size();
            if (needs_indent)
                sink(ib, ie);
            auto i0 = i;
            while (i != e)
            {
                if (*i++ == '\n')
                {
                    sink(i0, i);
                    sink(ib, ie);
                    i0 = i;
                }
            }
            needs_indent = *i++ == '\n';
            sink(i0, i);
        }
        
        void operator()(ast::variable const& variable)
        {
            if (auto pv = resolve(variable.key))
            {
                if (needs_indent)
                {
                    sink(indent.data(), indent.data() + indent.size());
                    needs_indent = false;
                }
                variable_visitor<content_visitor> visitor
                {
                    *this, escaping && !variable.tag
                };
                apply_visitor(visitor, *pv);
            }
        }
        
        void operator()(ast::section const& section)
        {
            bool inverted = section.tag == '^';
            auto old_cursor = cursor;
            if (auto next = resolve(section.key))
            {
                cursor = next;
                section_visitor<content_visitor> visitor
                {
                    *this, section.contents, inverted
                };
                if (!apply_visitor(visitor, *cursor))
                {
                    cursor = old_cursor;
                    return;
                }
            }
            else if (!inverted)
                return;
                
            for (auto const& content : section.contents)
                apply_visitor(*this, content);
            cursor = old_cursor;
        }
        
        void operator()(ast::partial const& partial)
        {
            auto it = context.find(partial.key);
            if (it != context.end())
            {
                if (it->second.contents().empty())
                    return;

                auto old_size = indent.size();
                auto old_needs_indent = needs_indent;
                indent += partial.indent;
                needs_indent = !indent.empty();
                for (auto const& content : it->second.contents())
                    apply_visitor(*this, content);
                needs_indent = old_needs_indent;
                indent.resize(old_size);
            }
        }
        
        void operator()(ast::null) const {} // never called
    };

    template<class CharT, class Traits>
    struct ostream_sink
    {
        std::basic_ostream<CharT, Traits>& out;
        
        void operator()(char const* it, char const* end) const
        {
            out.write(it, end - it);
        }

        template<class T>
        void operator()(T data) const
        {
            out << data;
        }

        void operator()(bool data) const
        {
            out << (data ? "true" : "false");
        }
    };

    template<class String>
    struct string_sink
    {
        String& out;

        void operator()(char const* it, char const* end) const
        {
            out.insert(out.end(), it, end);
        }

        void operator()(int data) const
        {
            append_num("%d", data);
        }

        void operator()(double data) const
        {
            append_num("%g", data);
        }

        void operator()(bool data) const
        {
            data ? append("true") : append("false");
        }

        template<std::size_t N>
        void append(char const (&str)[N]) const
        {
            out.insert(out.end(), str, str + (N - 1));
        }

        template<class T>
        void append_num(char const* fmt, T data) const
        {
            char buf[64];
            char* p;
            auto old_size = out.size();
            auto capacity = out.capacity();
            auto bufsize = capacity - old_size;
            if (bufsize)
            {
                out.resize(capacity);
                p = &out.front() + old_size;
            }
            else
            {
                bufsize = sizeof(buf);
                p = buf;
            }
            auto n = std::snprintf(p, bufsize, fmt, data);
            if (n < 0) // error
                return;
            if (n + 1 <= bufsize)
            {
                if (p == buf)
                {
                    out.insert(out.end(), p, p + n);
                    return;
                }
            }
            else
            {
                out.resize(old_size + n + 1); // '\0' will be written
                std::snprintf(&out.front() + old_size, n + 1, fmt, data);
            }
            out.resize(old_size + n);
        }
    };
}}

namespace bustache
{
    template<class Sink>
    inline void generate
    (
        format const& fmt, value::view const& data, Sink const& sink
      , option_type flag = normal
    )
    {
        generate(fmt, data, no_context::dummy(), sink, flag);
    }
    
    template<class Context, class Sink>
    void generate
    (
        format const& fmt, value::view const& data, Context const& context
      , Sink const& sink, option_type flag = normal
    )
    {
        object const empty;
        auto obj = get<object>(&data);
        detail::content_scope scope{nullptr, obj ? *obj : empty};
        detail::content_visitor<Sink, Context> visitor(scope, data.get_pointer(), sink, context, flag);
        for (auto const& content : fmt.contents())
            apply_visitor(visitor, content);
    }

    template<class CharT, class Traits, class T, class Context,
        std::enable_if_t<std::is_constructible<value::view, T>::value, bool> = true>
    inline std::basic_ostream<CharT, Traits>&
    operator<<(std::basic_ostream<CharT, Traits>& out, manipulator<T, Context> const& manip)
    {
        detail::ostream_sink<CharT, Traits> sink{out};
        generate(manip.fmt, manip.data, manip.context, sink, manip.flag);
        return out;
    }

    template<class T, class Context,
        std::enable_if_t<std::is_constructible<value::view, T>::value, bool> = true>
    inline std::string to_string(manipulator<T, Context> const& manip)
    {
        std::string ret;
        detail::string_sink<std::string> sink{ret};
        generate(manip.fmt, manip.data, manip.context, sink, manip.flag);
        return ret;
    }
}

#endif