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
    
    enum option_type : std::uint8_t
    {
        normal, escape_html
    };
    
    struct no_context_t {};

    static constexpr no_context_t no_context {};

    template<class T>
    struct context_trait
    {
        static format const* get(T const& self, std::string const& key)
        {
            auto it = self.find(key);
            return it == self.end() ? nullptr : &it->second;
        }
    };

    template<>
    struct context_trait<no_context_t>
    {
        static format const* get(no_context_t, std::string const&)
        {
            return nullptr;
        }
    };

    template<class T, class Context>
    struct manipulator
    {
        format const& fmt;
        T const& data;
        Context const& context;
        option_type const flag;
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

        format(char const* begin, char const* end)
        {
            init(begin, end);
        }
        
        template<class Source>
        explicit format(Source const& source)
        {
            init(source.data(), source.data() + source.size());
        }

        template<class Source>
        format(Source const& source, bool copytext)
        {
            init(source.data(), source.data() + source.size());
            if (copytext)
                copy_text(text_size());
        }

        template<class Source>
        explicit format(Source const&& source) = delete;

        template<std::size_t N>
        explicit format(char const (&source)[N])
        {
            init(source, source + (N - 1));
        }

        explicit format(ast::content_list contents, bool copytext = true)
          : _contents(std::move(contents))
        {
            if (copytext)
                copy_text(text_size());
        }

        format(format&& other) noexcept
          : _contents(std::move(other._contents)), _text(std::move(other._text))
        {}

        format(format const& other) : _contents(other._contents)
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
        manipulator<T, no_context_t>
        operator()(T const& data, option_type flag = normal) const
        {
            return {*this, data, no_context, flag};
        }
        
        template<class T, class Context>
        manipulator<T, Context>
        operator()(T const& data, Context const& context, option_type flag = normal) const
        {
            return {*this, data, context, flag};
        }
        
        ast::content_list const& contents() const
        {
            return _contents;
        }
        
    private:
        void init(char const* begin, char const* end);
        std::size_t text_size() const noexcept;
        void copy_text(std::size_t n);

        ast::content_list _contents;
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