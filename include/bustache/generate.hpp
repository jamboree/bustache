/*//////////////////////////////////////////////////////////////////////////////
    Copyright (c) 2016-2018 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////*/
#ifndef BUSTACHE_GENERATE_HPP_INCLUDED
#define BUSTACHE_GENERATE_HPP_INCLUDED

#include <bustache/model.hpp>

namespace bustache { namespace detail
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

    struct array_cursor
    {
        list_view arr;
        std::uintptr_t state;

        array_cursor(list_view arr) : arr(arr)
        {
            state = arr.begin_cursor();
        }

        ~array_cursor()
        {
            arr.end_cursor(state);
        }

        value_ptr next(value_holder& hold)
        {
            return arr.next(state, hold);
        }
    };

    template<class Sink>
    struct value_printer
    {
        Sink const& sink;
        bool const escaping;

        void operator()(std::nullptr_t) const {}

        void operator()(bool data) const { sink(data); }

        void operator()(int data) const { sink(data); }

        void operator()(double data) const { sink(data); }

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
                bustache::visit(*this, *it);
                while (++it != end)
                {
                    literal(",");
                    bustache::visit(*this, *it);
                }
            }
        }

        void operator()(array_cursor cursor) const
        {
            {
                value_holder hold;
                if (auto pv = cursor.next(hold))
                    bustache::visit(*this, *pv);
                else
                    return;
            }
            for (;;)
            {
                value_holder hold;
                if (auto pv = cursor.next(hold))
                {
                    literal(",");
                    bustache::visit(*this, *pv);
                }
                else
                    break;
            }
        }

        void operator()(object_view) const
        {
            literal("[Object]");
        }

        void operator()(lambda0v const& data) const
        {
            bustache::visit(*this, data());
        }

        void operator()(lambda1v const& data) const
        {
            bustache::visit(*this, data({}));
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
                if (auto str = get_escaped(*it))
                {
                    sink(last, it);
                    literal(str);
                    last = ++it;
                }
                else
                    ++it;
            }
            sink(last, it);
        }

        void literal(strlit str) const
        {
            sink(str.data, str.data + str.size);
        }
    };

    struct content_scope
    {
        content_scope const* const parent;
        object_view data;

        value_ptr lookup(std::string const& key, value_holder& hold) const
        {
            if (auto pv = data.get(key, hold))
                return pv;
            if (parent)
                return parent->lookup(key, hold);
            return nullptr;
        }
    };

    struct resolve_result
    {
        value_ptr pv;
        char const* sub;
    };

    struct resolve_handler
    {
        template<class F>
        resolve_handler(F const& f) noexcept : fn(&fn_impl<F>), p(&f) {}

        void operator()(value_view v) const { fn(p, v); }

        template<class F>
        static void fn_impl(void const* p, value_view v)
        {
            (*static_cast<F const*>(p))(v);
        }

        void(*fn)(void const*, value_view);
        void const* p;
    };

    struct nested_resolver
    {
        using iter = char const*;
        std::string& key_cache;
        object_view obj;
        resolve_handler handle;

        bool next(iter k0, iter ke, value_holder& hold);
        bool done(value_holder& hold);
    };

    struct content_visitor_base
    {
        using result_type = void;

        content_scope const* scope;
        value_ptr cursor;
        std::vector<ast::override_map const*> chain;
        mutable std::string key_cache;

        // Defined in src/generate.cpp.
        resolve_result resolve(std::string const& key, value_holder& hold) const;

        ast::content_list const* find_override(std::string const& key) const
        {
            for (auto pm : chain)
            {
                auto it = pm->find(key);
                if (it != pm->end())
                    return &it->second;
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
                bustache::visit(parent, content);
        }
    };

    template<class ContentVisitor>
    struct section_visitor
    {
        using result_type = bool;

        ContentVisitor& parent;
        ast::content_list const& contents;
        bool const inverted;

        void expand() const
        {
            for (auto const& content : contents)
                bustache::visit(parent, content);
        }

        void expand_with_scope(object_view data) const
        {
            content_scope scope{parent.scope, data};
            parent.scope = &scope;
            expand();
            parent.scope = scope.parent;
        }

        bool operator()(object_view data) const
        {
            if (!inverted)
                expand_with_scope(data);
            return false;
        }

        bool operator()(array const& data) const
        {
            if (inverted)
                return data.empty();

            for (auto const& val : data)
            {
                parent.cursor = val.get_pointer();
                if (auto obj = get_object(parent.cursor))
                    expand_with_scope(obj);
                else
                    expand();
            }
            return false;
        }

        bool operator()(list_view data) const
        {
            if (inverted)
                return data.empty();

            for (array_cursor cursor(data);;)
            {
                value_holder hold;
                if (auto pv = cursor.next(hold))
                {
                    parent.cursor = pv;
                    if (auto obj = get_object(pv))
                        expand_with_scope(obj);
                    else
                        expand();
                }
                else
                    break;
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
            return inverted ? false : bustache::visit(*this, data());
        }

        bool operator()(lambda0f const& data) const
        {
            if (!inverted)
            {
                auto fmt(data());
                for (auto const& content : fmt.contents())
                    bustache::visit(parent, content);
            }
            return false;
        }

        bool operator()(lambda1v const& data) const
        {
            return inverted ? false : bustache::visit(*this, data(contents));
        }

        bool operator()(lambda1f const& data) const
        {
            if (!inverted)
            {
                auto fmt(data(contents));
                for (auto const& content : fmt.contents())
                    bustache::visit(parent, content);
            }
            return false;
        }
    };

    template<class Sink, class Context, class UnresolvedHandler>
    struct content_visitor : content_visitor_base
    {
        using sink_type = Sink;

        Sink const& sink;
        Context const& context;
        UnresolvedHandler handle_unresolved;
        std::string indent;
        bool needs_indent;
        bool const escaping;

        content_visitor
        (
            content_scope const& scope, value_ptr cursor,
            Sink const &sink, Context const &context,
            UnresolvedHandler&& f, bool escaping
        )
          : content_visitor_base{&scope, cursor, {}, {}}
          , sink(sink), context(context)
          , handle_unresolved(std::forward<UnresolvedHandler>(f))
          , needs_indent(), escaping(escaping)
        {}

        void handle_variable(ast::variable const& variable, value_view val)
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
            bustache::visit(visitor, val);
        }

        void handle_section(ast::section const& section, value_view val)
        {
            bool inverted = section.tag == '^';
            auto old_cursor = cursor;
            section_visitor<content_visitor> visitor
            {
                *this, section.contents, inverted
            };
            cursor = val.get_pointer();
            if (bustache::visit(visitor, val))
            {
                for (auto const& content : section.contents)
                    bustache::visit(*this, content);
            }
            cursor = old_cursor;
        }

        template<class Handler>
        void resolve_and_handle(std::string const& key, Handler handle)
        {
            value_holder hold;
            auto result = resolve(key, hold);
            if (result.sub)
            {
                if (auto obj = get_object(result.pv))
                {
                    nested_resolver nested{key_cache, obj, handle};
                    if (nested.next(result.sub, key.data() + key.size(), hold))
                        return;
                }
            }
            else if (result.pv)
                return handle(*result.pv);
            handle(handle_unresolved(key));
        }

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
            resolve_and_handle(variable.key, [&](value_view val)
            {
                handle_variable(variable, val);
            });
        }

        void operator()(ast::section const& section)
        {
            resolve_and_handle(section.key, [&](value_view val)
            {
                handle_section(section, val);
            });
        }
        
        void operator()(ast::partial const& partial)
        {
            if (auto p = context_trait<Context>::get(context, partial.key))
            {
                if (p->contents().empty())
                    return;

                auto old_size = indent.size();
                auto old_chain = chain.size();
                indent += partial.indent;
                needs_indent |= !partial.indent.empty();
                if (!partial.overriders.empty())
                    chain.push_back(&partial.overriders);
                for (auto const& content : p->contents())
                    bustache::visit(*this, content);
                chain.resize(old_chain);
                indent.resize(old_size);
            }
        }

        void operator()(ast::block const& block)
        {
            auto pc = find_override(block.key);
            if (!pc)
                pc = &block.contents;
            for (auto const& content : *pc)
                bustache::visit(*this, content);
        }

        void operator()(ast::null) const {} // never called
    };
}}

namespace bustache
{
    template<class Sink, class UnresolvedHandler = default_unresolved_handler>
    inline void generate
    (
        Sink& sink, format const& fmt, value_view const& data,
        option_type flag = normal, UnresolvedHandler&& f = {}
    )
    {
        generate(sink, fmt, data, no_context, flag, std::forward<UnresolvedHandler>(f));
    }
    
    template<class Sink, class Context, class UnresolvedHandler = default_unresolved_handler>
    void generate
    (
        Sink& sink, format const& fmt, value_view const& data,
        Context const& context, option_type flag = normal, UnresolvedHandler&& f = {}
    )
    {
        detail::content_scope scope{nullptr, get_object(data.get_pointer())};
        detail::content_visitor<Sink, Context, UnresolvedHandler> visitor
        {
            scope, data.get_pointer(), sink, context,
            std::forward<UnresolvedHandler>(f), bool(flag)
        };
        for (auto const& content : fmt.contents())
            bustache::visit(visitor, content);
    }
}

#endif
