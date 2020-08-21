/*//////////////////////////////////////////////////////////////////////////////
    Copyright (c) 2016-2020 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////*/
#ifndef BUSTACHE_DEBUG_HPP_INCLUDED
#define BUSTACHE_DEBUG_HPP_INCLUDED

#include <iostream>
#include <iomanip>
#include <bustache/format.hpp>

namespace bustache::detail
{
    constexpr char const* get_tag_str(ast::type tag)
    {
        switch (tag)
        {
        case ast::type::var_raw: return "(&)";
        case ast::type::section: return "(#)";
        case ast::type::inversion: return "(^)";
        case ast::type::filter: return "(?)";
        case ast::type::loop: return "(*)";
        case ast::type::inheritance: return "($)";
        }
        return "";
    }

    template<class CharT, class Traits>
    struct ast_printer
    {
        ast::context const& ctx;
        std::basic_ostream<CharT, Traits>& out;
        unsigned level;
        unsigned const space;

        void write_text(std::string_view s) const
        {
            auto i = s.data();
            auto i0 = i;
            auto const e = i + s.size();
            out << '"';
            while (i != e)
            {
                char const* esc = nullptr;
                switch (*i)
                {
                case '\r': esc = "\\r"; break;
                case '\n': esc = "\\n"; break;
                case '\\': esc = "\\\\"; break;
                default: ++i; continue;
                }
                out.write(i0, i - i0);
                i0 = ++i;
                out << esc;
            }
            out.write(i0, i - i0);
            out << '"';
        }

        void operator()(ast::type, ast::text const* text) const
        {
            indent();
            out << "text: ";
            write_text(*text);
            out << "\n";
        }

        void operator()(ast::type tag, ast::variable const* variable) const
        {
            indent();
            out << "variable" << get_tag_str(tag);
            out << ": " << variable->key << "\n";
        }

        void operator()(ast::type tag, ast::block const* block)
        {
            out << "section" << get_tag_str(tag) << ": " << block->key << "\n";
            ++level;
            for (auto const& content : block->contents)
                ctx.visit(*this, content);
            --level;
        }

        void operator()(ast::type, ast::partial const* partial) const
        {
            out << "partial: " << partial->key << ", indent: ";
            write_text(partial->indent);
            out << "\n";
        }

        void operator()(ast::type, void const*) const {} // never called

        void indent() const
        {
            out << std::setw(space * level) << "";
        }
    };
}

namespace bustache
{
    template<class CharT, class Traits>
    inline void print_ast(std::basic_ostream<CharT, Traits>& out, format const& fmt, unsigned indent = 4)
    {
        auto const view = fmt.view();
        detail::ast_printer<CharT, Traits> visitor{view.ctx, out, 0, indent};
        for (auto const& content : view.contents)
            view.ctx.visit(visitor, content);
    }
}

#endif