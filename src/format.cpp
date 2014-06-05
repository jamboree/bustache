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
    
    using x3::seek;
    using x3::lexeme;
    using x3::skip;
    using x3::raw;
    using x3::until;

    x3::rule<class start, ast::content_list> const start;
    x3::rule<class text, boost::string_ref> const text;
    x3::rule<class id, std::string> const id;
    x3::rule<class variable, ast::variable> const variable;
    x3::rule<class section, ast::section> const section;
    x3::rule<class partial, ast::partial> const partial;
    x3::rule<class content, ast::content> const content;
    
    x3::as<boost::string_ref> const as_text;
    
    struct get_id
    {
        template <typename Context>
        std::string const& operator()(Context const& ctx) const
        {
            return x3::_val(ctx).id;
        }
    };

    BOOST_SPIRIT_DEFINE
    (
        start =
            *(content | as_text[string])
            
      , text =
            lexeme[raw[seek[char_ >> &skip["{{"]]]]
            
      , id =
            lexeme[raw[seek[char_ >> &skip["}}"]]]]

      , variable =
            id >> "}}"
            
      , section =
                char_("#^") >> id >> "}}"
            >>  until(lit("{{") >> '/' >> lit(get_id()) >> "}}")
                [
                    content
                ]
        
      , partial =
            '>' >> id >> "}}"

      , content =
                "{{" >> (section | partial | variable)
            |   text
    )
}}

namespace bustache
{
    format::format(char const* begin, char const* end)
    {
        x3::phrase_parse(begin, end, parser::start, x3::space, _contents);
    }
    
    std::size_t format::text_size() const
    {
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
            
            std::size_t operator()(std::string const&) const
            {
                return 0;
            }
        } accum;
        std::size_t n = 0;
        for (auto const& content : _contents)
            n += boost::apply_visitor(accum, content);
        return n;
    }
    
    void format::copy_text(std::size_t n)
    {
        _text.reserve(n);
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
            
            void operator()(std::string const&) const {}
        } insert{_text};
        for (auto& content : _contents)
            boost::apply_visitor(insert, content);
    }
}
