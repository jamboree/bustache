/*//////////////////////////////////////////////////////////////////////////////
    Copyright (c) 2016-2020 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////*/

#include <bustache/render.hpp>
#include <cassert>

namespace bustache::detail
{
    struct object_ptr
    {
        std::uintptr_t data;
        void(*_get)(std::uintptr_t self, std::string const& key, value_handler visit);

        explicit operator bool() const noexcept
        {
            return !!data;
        }

        void get(std::string const& key, value_handler visit) const
        {
            _get(data, key, visit);
        }
    };

    inline object_ptr get_object(value_ptr val)
    {
        if (val.vptr->kind == model::object)
            return {val.data, static_cast<value_vtable const*>(val.vptr)->get};
        return {0, object_trait::get_default};
    }

    struct content_scope
    {
        content_scope const* const parent;
        object_ptr data;
    };

    template<class Visit>
    void lookup(content_scope const* scope, std::string const& key, Visit const& visit)
    {
        bool found = false;
        do
        {
            scope->data.get(key, [&](value_ptr val)
            {
                if (val)
                {
                    visit(val);
                    found = true;
                }
            });
            if (found)
                return;
            scope = scope->parent;
        } while (scope);
        visit(nullptr);
    }

    struct nested_resolver
    {
        using iter = char const*;
        iter ki;
        iter const ke;
        std::string& key_cache;
        value_handler handle;
        bool done;

        void next(object_ptr obj)
        {
            auto const k0 = ++ki;
            while (ki != ke)
            {
                if (*ki == '.')
                {
                    key_cache.assign(k0, ki);
                    return obj.get(key_cache, [this](value_ptr val)
                    {
                        if (auto const obj = get_object(val))
                            next(obj);
                    });
                }
                else
                    ++ki;
            }
            key_cache.assign(k0, ki);
            obj.get(key_cache, [this](value_ptr val)
            {
                if (val)
                {
                    handle(val);
                    done = true;
                }
            });
        }
    };

    struct content_visitor
    {
        using result_type = void;

        content_scope const* scope;
        value_ptr cursor;
        std::vector<ast::override_map const*> chain;
        mutable std::string key_cache;

        output_handler raw_os;
        output_handler escape_os;
        context_handler context;
        unresolved_handler variable_unresolved;
        std::string indent;
        bool needs_indent;

        content_visitor
        (
            content_scope const& scope, value_ptr cursor,
            output_handler raw_os, output_handler escape_os, context_handler context,
            unresolved_handler f
        )
            : scope(&scope), cursor(cursor)
            , raw_os(raw_os), escape_os(escape_os), context(context)
            , variable_unresolved(f)
            , needs_indent()
        {}

        template<class Visit>
        void resolve(std::string const& key, Visit visit) const
        {
            auto ki = key.data();
            auto const ke = ki + key.size();
            if (ki == ke)
                return visit(nullptr, nullptr);
            struct on_value
            {
                Visit const& visit;
                char const* sub;
                on_value(Visit const& visit, char const* ki, char const* ke) noexcept
                    : visit(visit), sub(ki == ke ? nullptr : ki)
                {}
                void operator()(value_ptr val) const
                {
                    visit(val, sub);
                };
            };
            if (*ki == '.')
            {
                if (++ki == ke)
                    return visit(cursor, nullptr);
                auto k0 = ki;
                while (*ki != '.' && ++ki != ke);
                key_cache.assign(k0, ki);
                scope->data.get(key_cache, on_value(visit, ki, ke));
            }
            else
            {
                auto k0 = ki;
                while (ki != ke && *ki != '.') ++ki;
                key_cache.assign(k0, ki);
                lookup(scope, key_cache, on_value(visit, ki, ke));
            }
        }

        ast::content_list const* find_override(std::string const& key) const;

        void print_value(output_handler os, value_ptr val);

        void handle_variable(ast::variable const& variable, value_ptr val);

        void expand(ast::content_list const& contents)
        {
            for (auto const& content : contents)
                visit(*this, content);
        }

        void expand_on_object(ast::content_list const& contents, object_ptr data)
        {
            content_scope curr{scope, data};
            scope = &curr;
            expand(contents);
            scope = curr.parent;
        }

        void expand_on_value(ast::content_list const& contents, value_ptr val)
        {
            if (auto obj = get_object(val))
                expand_on_object(contents, obj);
            else
                expand(contents);
        }

        bool expand_section(char tag, ast::content_list const& contents, value_ptr val);

        void handle_section(ast::section const& section, value_ptr val);

        void resolve_and_handle(std::string const& key, unresolved_handler unresolved, value_handler handle);

        void operator()(ast::text const& text);

        void operator()(ast::variable const& variable)
        {
            resolve_and_handle(variable.key, variable_unresolved, [&](value_ptr val)
            {
                handle_variable(variable, val);
            });
        }

        void operator()(ast::section const& section)
        {
            resolve_and_handle(section.key, nullptr, [&](value_ptr val)
            {
                handle_section(section, val);
            });
        }

        void operator()(ast::partial const& partial);

        void operator()(ast::inheritance const& inheritance)
        {
            auto pc = find_override(inheritance.key);
            if (!pc)
                pc = &inheritance.contents;
            for (auto const& content : *pc)
                visit(*this, content);
        }

        void operator()(ast::null) const {} // never called
    };

    ast::content_list const* content_visitor::find_override(std::string const& key) const
    {
        for (auto pm : chain)
        {
            auto it = pm->find(key);
            if (it != pm->end())
                return &it->second;
        }
        return nullptr;
    }

    void content_visitor::print_value(output_handler os, value_ptr val)
    {
        switch (val.vptr->kind)
        {
        case model::lazy_value:
            static_cast<lazy_value_vtable const*>(val.vptr)->call(val.data, nullptr, [=](value_ptr val)
            {
                print_value(os, val);
            });
            break;
        case model::lazy_format:
        {
            auto const fmt = static_cast<lazy_format_vtable const*>(val.vptr)->call(val.data, nullptr);
            for (auto const& content : fmt.contents())
                visit(*this, content);
            break;
        }
        default:
            static_cast<value_vtable const*>(val.vptr)->print(val.data, os, nullptr);
        }
    }

    void content_visitor::handle_variable(ast::variable const& variable, value_ptr val)
    {
        if (needs_indent)
        {
            raw_os(indent.data(), indent.size());
            needs_indent = false;
        }
        print_value(variable.tag ? raw_os : escape_os, val);
    }

    bool content_visitor::expand_section(char tag, ast::content_list const& contents, value_ptr val)
    {
        bool inverted = false;
        auto kind = val.vptr->kind;
        if (kind < model::lazy_value)
        {
            switch (tag)
            {
            case '?':
                kind = model::atom;
                break;
            case '*':
                kind = model::list;
                break;
            case '^':
                inverted = true;
                break;
            }
        }
        else if (tag == '^') // Inverted lazy.
            return false;
        switch (kind)
        {
        case model::null:
            return inverted;
        case model::atom:
            return static_cast<value_vtable const*>(val.vptr)->test(val.data) ^ inverted;
        case model::object:
            if (!inverted)
                expand_on_object(contents, {val.data, static_cast<value_vtable const*>(val.vptr)->get});
            return false;
        case model::list:
        {
            auto const vt = static_cast<value_vtable const*>(val.vptr);
            if (inverted)
                return !static_cast<value_vtable const*>(val.vptr)->test(val.data);
            if (!vt->iterate)
                expand_on_value(contents, val);
            else
            {
                vt->iterate(val.data, [&](value_ptr val)
                {
                    cursor = val;
                    expand_on_value(contents, val);
                });
            }
            return false;
        }
        case model::lazy_value:
        {
            bool ret = false;
            static_cast<lazy_value_vtable const*>(val.vptr)->call(val.data, &contents, [&](value_ptr val)
            {
                ret = expand_section(tag, contents, val);
            });
            return ret;
        }
        case model::lazy_format:
        {
            if (tag == '?')
                return true;
            auto const fmt = static_cast<lazy_format_vtable const*>(val.vptr)->call(val.data, &contents);
            for (auto const& content : fmt.contents())
                visit(*this, content);
            return false;
        }
        }
        std::abort(); // Unreachable.
    }

    void content_visitor::handle_section(ast::section const& section, value_ptr val)
    {
        auto old_cursor = cursor;
        cursor = val;
        if (expand_section(section.tag, section.contents, val))
        {
            for (auto const& content : section.contents)
                visit(*this, content);
        }
        cursor = old_cursor;
    }

    void content_visitor::resolve_and_handle(std::string const& key, unresolved_handler unresolved, value_handler handle)
    {
        resolve(key, [&](value_ptr val, char const* sub)
        {
            if (sub)
            {
                if (auto obj = get_object(val))
                {
                    nested_resolver nested{sub, key.data() + key.size(), key_cache, handle};
                    if (nested.next(obj), nested.done)
                        return;
                }
            }
            else if (val)
                return handle(val);
            handle(unresolved ? unresolved(key) : nullptr);
        });
    }

    void content_visitor::operator()(ast::text const& text)
    {
        auto i = text.data();
        auto const n = text.size();
        assert(n && "empty text shouldn't be in ast");
        if (indent.empty())
        {
            raw_os(i, n);
            return;
        }
        auto const ib = indent.data();
        auto const in = indent.size();
        if (needs_indent)
            raw_os(ib, in);
        auto i0 = i;
        auto const e = i + (n - 1); // Don't flush indent on last newline.
        while (i != e)
        {
            if (*i++ == '\n')
            {
                raw_os(i0, i - i0);
                raw_os(ib, in);
                i0 = i;
            }
        }
        needs_indent = *i++ == '\n';
        raw_os(i0, i - i0);
    }

    void content_visitor::operator()(ast::partial const& partial)
    {
        if (auto p = context(partial.key))
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
                visit(*this, content);
            chain.resize(old_chain);
            indent.resize(old_size);
        }
    }

    void render(output_handler raw_os, output_handler escape_os, format const& fmt, value_ptr data, context_handler context, unresolved_handler f)
    {
        content_scope scope{nullptr, get_object(data)};
        content_visitor visitor
        {
            scope, data, raw_os, escape_os, context, f
        };
        for (auto const& content : fmt.contents())
            visit(visitor, content);
    }
}