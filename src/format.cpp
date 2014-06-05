/*//////////////////////////////////////////////////////////////////////////////
    Copyright (c) 2014 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////*/
#include <bustache/format.hpp>
#include <boost/spirit/home/x3.hpp>

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
    using x3::raw;

    x3::rule<class content, ast::content> const content;
    x3::rule<class text, boost::string_ref> const text;
    x3::rule<class id, std::string(char const*)> const id;
    x3::rule<class variable, ast::variable> const variable;
    x3::rule<class section, ast::section> const section;
    x3::rule<class partial, ast::partial> const partial;
    x3::rule<class comment, ast::comment> const comment;
    
    x3::as<boost::string_ref> const as_text;
    
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
        content =
                "{{" >> (section | partial | comment | variable)
            |   text
            
      , text =
            no_skip[raw[+(char_ - "{{")]]
            
      , id =
                lexeme[raw[+(char_ - skip[lit(_r1) >> "}}"])]]
            >>  lit(_r1)

      , variable =
            (char_("&{") | !lit('/')) >> id(esc()) >> "}}"
            
      , section =
                char_("#^") >> id("") >> "}}"
            >>  *content
            >>  lit("{{") >> '/' >> lit(get_id()) >> "}}"

      , partial =
            '>' >> id("") >> "}}"
        
      , comment =
            '!' >> seek["}}"]
    )
}}

namespace bustache
{
    format::format(char const* begin, char const* end)
    {
        x3::phrase_parse(begin, end, *parser::content, x3::space, _contents);
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
