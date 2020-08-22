/*//////////////////////////////////////////////////////////////////////////////
    Copyright (c) 2014-2020 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////*/
#ifndef BUSTACHE_FORMAT_HPP_INCLUDED
#define BUSTACHE_FORMAT_HPP_INCLUDED

#include <bustache/ast.hpp>
#include <stdexcept>
#include <cstddef>
#include <utility>
#include <memory>

namespace bustache
{
    struct format;

    namespace detail
    {
        struct manip_base
        {
            format const& fmt;
        };

        template<class T>
        struct manip_core
        {
            T const& data;
        };

        template<class T>
        struct manip_context
        {
            T const& context;
        };

        template<class T>
        struct manip_escape
        {
            T const& escape;
        };
    }

    template<class... Opts>
    struct manipulator : detail::manip_base, Opts...
    {
        template<class T>
        manipulator<Opts..., detail::manip_context<T>> context(T const& context_) const noexcept
        {
            return {fmt, static_cast<Opts const&>(*this)..., context_};
        }

        template<class T>
        manipulator<Opts..., detail::manip_escape<T>> escape(T const& escape_) const noexcept
        {
            return {fmt, static_cast<Opts const&>(*this)..., escape_};
        }
    };

    enum error_type
    {
        error_set_delim,
        error_baddelim,
        error_delim,
        error_section,
        error_badkey
    };

    class format_error : public std::runtime_error
    {
        error_type _err;
        std::ptrdiff_t _pos;

    public:
        format_error(error_type err, std::ptrdiff_t position);

        error_type code() const noexcept { return _err; }
        std::ptrdiff_t position() const noexcept { return _pos; }
    };

    struct format
    {
        format() = default;

        explicit format(std::string_view source)
        {
            init(source.data(), source.data() + source.size());
        }

        format(std::string_view source, bool copytext)
        {
            init(source.data(), source.data() + source.size());
            if (copytext)
                copy_text(text_size());
        }

        format(ast::document doc, bool copytext)
          : _doc(std::move(doc))
        {
            if (copytext)
                copy_text(text_size());
        }

        format(format&& other) = default;

        format(format const& other) : _doc(other._doc)
        {
            if (other._text)
                copy_text(text_size());
        }

        format& operator=(format&& other) = default;

        format& operator=(format const& other)
        {
            return operator=(format(other));
        }

        template<class T>
        manipulator<detail::manip_core<T>> operator()(T const& data) const
        {
            return {*this, data};
        }

        ast::document const& doc() const noexcept
        {
            return _doc;
        }
        
    private:
        void init(char const* begin, char const* end);
        std::size_t text_size() const noexcept;
        void copy_text(std::size_t n);

        ast::document _doc;
        std::unique_ptr<char[]> _text;
    };

    inline namespace literals
    {
        inline format operator"" _fmt(char const* str, std::size_t n)
        {
            return format(str, str + n);
        }
    }
}

#endif