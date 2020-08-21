/*//////////////////////////////////////////////////////////////////////////////
    Copyright (c) 2014-2020 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////*/
#include <cassert>
#include <utility>
#include <cstring>
#include <exception>
#include <bustache/format.hpp>

namespace bustache::parser { namespace
{
    using I = char const*;

    struct delim
    {
        std::string_view open;
        std::string_view close;
    };

    constexpr bool is_space(char c)
    {
        switch (c)
        {
        case ' ':
        case '\f':
        case '\n':
        case '\r':
        case '\t':
        case '\v':
            return true;
        }
        return false;
    }

    // Return true if it ends.
    inline bool skip(I& i, I e) noexcept
    {
        while (i != e)
        {
            if (!is_space(*i))
                return false;
            ++i;
        }
        return true;
    }

    inline bool parse_end_brace(I& i, I e) noexcept
    {
        if (i != e && *i == '}')
        {
            skip(++i, e);
            return true;
        }
        return false;
    }

    inline bool parse_lit(I& i, I e, std::string_view str) noexcept
    {
        if (e - i < std::ptrdiff_t(str.size()))
            return false;
        I p = i;
        for (char c : str)
        {
            if (*p != c)
                return false;
            ++p;
        }
        i = p;
        return true;
    }

    void expect_key(I b, I& i, I e, delim& d, std::string& attr, bool not_brace)
    {
        skip(i, e);
        for (I const i0 = i; i != e; ++i)
        {
            I const i1 = i;
            skip(i, e);
            if (not_brace || parse_end_brace(i, e))
            {
                if (parse_lit(i, e, d.close))
                {
                    if (i0 == i1)
                        throw format_error(error_badkey, i - b);
                    attr.assign(i0, i1);
                    return;
                }
            }
            if (i == e)
                break;
        }
        throw format_error(error_badkey, i - b);
    }

    I process_pure(I& i, I e, bool& pure) noexcept
    {
        I i0 = i;
        if (pure)
        {
            while (i != e)
            {
                if (*i == '\n')
                {
                    i0 = ++i;
                    break;
                }
                else if (is_space(*i))
                    ++i;
                else
                {
                    pure = false;
                    break;
                }
            }
        }
        return i0;
    }

    struct tag_result
    {
        bool is_end_section;
        bool check_standalone;
        bool is_standalone;
    };

    struct parser
    {
        ast::context& ctx;

        void parse_start(I& i, I e, ast::content_list& attr)
        {
            delim d{"{{", "}}"};
            bool pure = true;
            parse_contents(i, i, i, e, d, pure, attr, {});
        }

        bool parse_content
        (
            I b, I& i0, I& i, I e, delim& d, bool& pure,
            std::string_view& text, ast::content& attr,
            std::string_view section
        );

        void parse_contents
        (
            I b, I i0, I& i, I e, delim& d, bool& pure,
            ast::content_list& attr, std::string_view section
        );

        bool expect_block(I b, I& i, I e, delim& d, bool& pure, ast::block& attr)
        {
            expect_key(b, i, e, d, attr.key, true);
            I i0 = process_pure(i, e, pure);
            bool standalone = pure;
            parse_contents(b, i0, i, e, d, pure, attr.contents, attr.key);
            return standalone;
        }

        bool expect_inheritance(I b, I& i, I e, delim& d, bool& pure, ast::partial& attr);

        tag_result expect_tag
        (
            I b, I& i, I e, delim& d, bool& pure,
            ast::content& attr, std::string_view section
        );
    };

    bool parser::expect_inheritance(I b, I& i, I e, delim& d, bool& pure, ast::partial& attr)
    {
        expect_key(b, i, e, d, attr.key, true);
        I i0 = process_pure(i, e, pure);
        bool standalone = pure;
        for (std::string_view text;;)
        {
            ast::content a;
            auto end = parse_content(b, i0, i, e, d, pure, text, a, attr.key);
            if (auto p = ctx.get_if<ast::type::inheritance>(a))
                attr.overriders.emplace(std::move(p->key), std::move(p->contents));
            if (end)
                break;
        }
        return standalone;
    }

    void expect_comment(I b, I& i, I e, delim& d)
    {
        while (!parse_lit(i, e, d.close))
        {
            if (i == e)
                throw format_error(error_delim, i - b);
            ++i;
        }
    }

    void expect_set_delim(I b, I& i, I e, delim& d)
    {
        skip(i, e);
        I i0 = i;
        for (;;)
        {
            if (i == e)
                throw format_error(error_baddelim, i - b);
            if (is_space(*i))
                break;
            ++i;
        }
        d.open = std::string_view(i0, i - i0);
        skip(i, e);
        i0 = i;
        I i1 = i;
        for (;; ++i)
        {
            if (i == e)
                throw format_error(error_set_delim, i - b);
            if (*i == '=')
            {
                i1 = i;
                break;
            }
            if (is_space(*i))
            {
                i1 = i;
                if (skip(++i, e) || *i != '=')
                    throw format_error(error_set_delim, i - b);
                break;
            }
        }
        if (i0 == i1)
            throw format_error(error_baddelim, i - b);
        skip(++i, e);
        if (!parse_lit(i, e, d.close))
            throw format_error(error_delim, i - b);
        d.close = std::string_view(i0, i1 - i0);
    }

    tag_result parser::expect_tag
    (
        I b, I& i, I e, delim& d, bool& pure,
        ast::content& attr, std::string_view section
    )
    {
        if (skip(i, e))
            throw format_error(error_badkey, i - b);
        tag_result ret{};
        ast::type kind;
        switch (*i)
        {
        case '#':
            kind = ast::type::section;
            {
            block:
                ast::block a;
                ret.is_standalone = expect_block(b, ++i, e, d, pure, a);
                attr = ctx.add(kind, std::move(a));
            }
            break;
        case '^':
            kind = ast::type::inversion;
            goto block;
        case '?':
            kind = ast::type::filter;
            goto block;
        case '*':
            kind = ast::type::loop;
            goto block;
        case '$':
            kind = ast::type::inheritance;
            goto block;
        case '/':
            skip(++i, e);
            if (!parse_lit(i, e, section))
                throw format_error(error_section, i - b);
            skip(i, e);
            if (!parse_lit(i, e, d.close))
                throw format_error(error_delim, i - b);
            ret.check_standalone = pure;
            ret.is_end_section = true;
            break;
        case '!':
        {
            expect_comment(b, ++i, e, d);
            ret.check_standalone = pure;
            break;
        }
        case '=':
        {
            expect_set_delim(b, ++i, e, d);
            ret.check_standalone = pure;
            break;
        }
        case '>':
        {
            ast::partial a;
            expect_key(b, ++i, e, d, a.key, true);
            attr = ctx.add(std::move(a));
            ret.check_standalone = pure;
            break;
        }
        case '&':
        case '{':
        {
            ast::variable a;
            expect_key(b, ++i, e, d, a.key, *i != '{');
            attr = ctx.add(ast::type::var_raw, std::move(a));
            pure = false;
            break;
        }
        // Extensions
        case '<':
        {
            ast::partial a;
            ret.is_standalone = expect_inheritance(b, ++i, e, d, pure, a);
            attr = ctx.add(std::move(a));
            return ret;
        }
        default:
            ast::variable a;
            expect_key(b, i, e, d, a.key, true);
            attr = ctx.add(ast::type::var_escaped, std::move(a));
            pure = false;
            break;
        }
        return ret;
    }

    // Return true if it ends.
    bool parser::parse_content
    (
        I b, I& i0, I& i, I e, delim& d, bool& pure,
        std::string_view& text, ast::content& attr,
        std::string_view section
    )
    {
        for (I i1 = i; i != e;)
        {
            if (*i == '\n')
            {
                pure = true;
                i1 = ++i;
            }
            else if (is_space(*i))
                ++i;
            else
            {
                I i2 = i;
                if (parse_lit(i, e, d.open))
                {
                    tag_result tag(expect_tag(b, i, e, d, pure, attr, section));
                    text = std::string_view(i0, i1 - i0);
                    if (tag.check_standalone)
                    {
                        I i3 = i;
                        while (i != e)
                        {
                            if (*i == '\n')
                            {
                                ++i;
                                break;
                            }
                            else if (is_space(*i))
                                ++i;
                            else
                            {
                                pure = false;
                                text = std::string_view(i0, i2 - i0);
                                // For end-section, we move the current pos (i)
                                // since i0 is local to the section and is not
                                // propagated upwards.
                                (tag.is_end_section ? i : i0) = i3;
                                return tag.is_end_section;
                            }
                        }
                        tag.is_standalone = true;
                    }
                    if (!tag.is_standalone)
                        text = std::string_view(i0, i2 - i0);
                    else if (auto partial = ctx.get_if<ast::type::partial>(attr))
                        partial->indent.assign(i1, i2 - i1);
                    i0 = i;
                    return i == e || tag.is_end_section;
                }
                else
                {
                    pure = false;
                    ++i;
                }
            }
        }
        text = std::string_view(i0, i - i0);
        return true;
    }

    void parser::parse_contents
    (
        I b, I i0, I& i, I e, delim& d, bool& pure,
        ast::content_list& attr, std::string_view section
    )
    {
        for (;;)
        {
            std::string_view text;
            ast::content a;
            auto end = parse_content(b, i0, i, e, d, pure, text, a, section);
            if (!text.empty())
                attr.push_back(ctx.add(text));
            if (!a.is_null())
                attr.push_back(std::move(a));
            if (end)
                return;
        }
    }
}}

namespace bustache
{
    static char const* get_error_string(error_type err) noexcept
    {
        switch (err)
        {
        case error_set_delim:
            return "mismatched '='";
        case error_baddelim:
            return "invalid delimiter";
        case error_delim:
            return "mismatched delimiter";
        case error_section:
            return "mismatched end section tag";
        case error_badkey:
            return "invalid key";
        default:
            assert(!"should not happen");
            std::terminate();
        }
    }

    format_error::format_error(error_type err, std::ptrdiff_t pos)
      : runtime_error(get_error_string(err)), _err(err), _pos(pos)
    {}

    void format::init(char const* begin, char const* end)
    {
        parser::parser{_doc.ctx}.parse_start(begin, end, _doc.contents);
    }

    std::size_t format::text_size() const noexcept
    {
        std::size_t n = 0;
        for (auto const& text : _doc.ctx.texts)
            n += text.size();
        return n;
    }

    void format::copy_text(std::size_t n)
    {
        if (n)
        {
            auto data = new char[n];
            _text.reset(data);
            for (auto& text : _doc.ctx.texts)
            {
                auto n = text.size();
                std::memcpy(data, text.data(), n);
                text = {data, n};
                data += n;
            }
        }
    }
}