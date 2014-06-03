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
    using x3::alpha;
    using x3::alnum;
    using x3::char_;
    using x3::eoi;
    using x3::string;
    
    using x3::seek;
    using x3::raw;

    x3::rule<class start, ast::content_list> const start;
    x3::rule<class text, boost::string_ref, x3::filter<>> const text;
    x3::rule<class term, std::string> const term;
    x3::rule<class block, ast::block> const block;
    x3::rule<class content, ast::content> const content;
    x3::rule<class id, std::string, x3::filter<>> const id;
    x3::rule<class end_block> const end_block;
    
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
            raw[seek[char_ >> &lit("{{")]]

      , term =
            id >> "}}"
            
      , block =
                '#' >> id >> "}}"
            >>  *(content - end_block)
            >>  end_block >> lit(get_id()) >> "}}"

      , content =
                "{{" >> (term | block)
            |   text
            
      , id =
            raw[(alpha | '_') >> *(alnum | '_')]
            
      , end_block =
            lit("{{") >> '/'
    )
}}

namespace bustache
{
    format::format(char const* begin, char const* end)
    {
        x3::phrase_parse(begin, end, parser::start, x3::space, contents);
    }
}
