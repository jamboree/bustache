/*//////////////////////////////////////////////////////////////////////////////
    Copyright (c) 2014 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////*/
#include <bustache/format.hpp>
#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>

namespace x3 = boost::spirit::x3;

namespace bustache { namespace parser
{
    using x3::lit;
    using x3::space;
    using x3::char_;
    using x3::string;

    using x3::lexeme;
    using x3::no_skip;
    using x3::skip;
    using x3::seek;
    using x3::sink;
    using x3::hold;
    using x3::with;
    using x3::raw;
    
    struct delim
    {
        typedef std::tuple<std::string, std::string> type;
        
        template <int N>
        struct get
        {
            template <typename Context>
            std::string const& operator()(Context const& ctx) const
            {
                return std::get<N>(x3::get<delim>(ctx));
            }
        };
        
        struct trans
        {
            static type pre(type const&)
            {
                return {};
            }
            
            static void post(type& copy, type& attr)
            {
                attr = std::move(copy);
            }
        };
        
        template <typename Context>
        type& operator()(Context const& ctx) const
        {
            return x3::get<delim>(ctx);
        }
    };
    
    typedef x3::filter<x3::skipper_tag, delim> filter;

    x3::rule<class start, ast::content_list> const start;
    x3::rule<class content, ast::content, filter> const content;
    x3::rule<class text, boost::string_ref, filter> const text;
    x3::rule<class id, std::string(char const*), filter> const id;
    x3::rule<class variable, ast::variable, filter> const variable;
    x3::rule<class section, ast::section, filter> const section;
    x3::rule<class partial, ast::partial, filter> const partial;
    x3::rule<class comment, ast::comment, filter> const comment;
    x3::rule<class set_delim, void, filter> const set_delim;
    
    x3::as<boost::string_ref> const as_text;
    auto const dL = lit(delim::get<0>());
    auto const dR = lit(delim::get<1>());
    
    struct get_id
    {
        template <typename Context>
        std::string const& operator()(Context const& ctx) const
        {
            return x3::_val(ctx).id;
        }
    };
    
    struct esc
    {
        template <typename Context>
        char const* operator()(Context const& ctx) const
        {
            return x3::_val(ctx).tag == '{'? "}" : "";
        }
    };

    x3::param_eval<0> const _r1;

    BOOST_SPIRIT_DEFINE
    (
        start =
            with<delim&>(delim::type{"{{", "}}"})
            [
                *content
            ]
            
      , content =
                dL >> (section | partial | comment | set_delim | variable)
            |   text
            
      , text =
            no_skip[raw[+(char_ - dL)]]
            
      , id =
                lexeme[raw[+(char_ - skip[lit(_r1) >> dR])]]
            >>  lit(_r1)

      , variable =
            (char_("&{") | !lit('/')) >> id(esc()) >> dR
            
      , section =
                char_("#^") >> id("") >> dR
            >>  *content
            >>  dL >> '/' >> lit(get_id()) >> dR

      , partial =
            '>' >> id("") >> dR
        
      , comment =
            '!' >> seek[dR]
        
      , set_delim =
            sink(delim())[hold(delim::trans())
            [
                '=' >> string >> lexeme[raw[+(~space - '=')]] >> '=' >> dR
            ]]
    )
}}

namespace bustache
{
    format::format(char const* begin, char const* end)
    {
        x3::phrase_parse(begin, end, parser::start, x3::space, _contents);
    }
    
    struct accum_size
    {
        typedef std::size_t result_type;

        std::size_t operator()(ast::text const& text) const
        {
            return text.size();
        }

        std::size_t operator()(ast::section const& section) const
        {
            std::size_t n = 0;
            for (auto const& content : section.contents)
                n += boost::apply_visitor(*this, content);
            return n;
        }
        
        template <typename T>
        std::size_t operator()(T const&) const
        {
            return 0;
        }
    };
    
    std::size_t format::text_size() const
    {
        accum_size accum;
        std::size_t n = 0;
        for (auto const& content : _contents)
            n += boost::apply_visitor(accum, content);
        return n;
    }

    struct insert_text
    {
        typedef void result_type;
        
        std::string& data;

        void operator()(ast::text& text) const
        {
            auto n = data.size();
            data.insert(data.end(), text.begin(), text.end());
            text = {data.data() + n, text.size()};
        }

        void operator()(ast::section& section) const
        {
            for (auto& content : section.contents)
                boost::apply_visitor(*this, content);
        }
        
        template <typename T>
        void operator()(T const&) const {}
    };
    
    void format::copy_text(std::size_t n)
    {
        _text.reserve(n);
        insert_text insert{_text};
        for (auto& content : _contents)
            boost::apply_visitor(insert, content);
    }
}
