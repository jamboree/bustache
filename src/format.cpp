/*//////////////////////////////////////////////////////////////////////////////
    Copyright (c) 2014 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////*/
#include <bustache/format.hpp>
#include <boost/spirit/home/x3.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/range/iterator_range.hpp>

namespace x3 = boost::spirit::x3;

namespace bustache
{
    struct flag_tag;
    
    void trim_contents(ast::content_list& contents)
    {
        using namespace boost::algorithm;
        if (!contents.empty())
        {
            if (auto p = boost::get<ast::text>(&contents.front()))
            {
                auto rng = boost::make_iterator_range(p->begin(), p->end());
                rng = trim_left_copy(rng);
                if (rng.empty())
                    contents.pop_front();
                else
                    *p = ast::text(rng.begin(), rng.end() - rng.begin());
            }
            if (auto p = boost::get<ast::text>(&contents.back()))
            {
                auto rng = boost::make_iterator_range(p->begin(), p->end());
                rng = trim_right_copy(rng);
                if (rng.empty())
                    contents.pop_back();
                else
                    *p = ast::text(rng.begin(), rng.end() - rng.begin());
            }
        }
    }
}

namespace bustache { namespace parser
{
    using x3::lit;
    using x3::space;
    using x3::char_;
    using x3::string;
    
    using x3::seek;
    using x3::lexeme;
    using x3::no_skip;
    using x3::skip;
    using x3::raw;
    using x3::until;
    
    typedef x3::filter<x3::skipper_tag, flag_tag> flag;

    x3::rule<class start, ast::content_list, flag> const start;
    x3::rule<class text, boost::string_ref> const text;
    x3::rule<class id, std::string> const id;
    x3::rule<class variable, ast::variable> const variable;
    x3::rule<class section, ast::section, flag> const section;
    x3::rule<class partial, ast::partial> const partial;
    x3::rule<class content, ast::content, flag> const content;
    
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
            no_skip[raw[seek[char_ >> &lit("{{")]]]
            
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

    template <typename Context>
    inline void
    on_success(decltype(section)&, char const*, char const*, ast::section& section, Context const& ctx)
    {
        if (x3::get<flag_tag>(ctx))
            trim_contents(section.contents);
    }
}}

namespace bustache
{
    format::format(char const* begin, char const* end, flag_type flag)
    {
        x3::phrase_parse(begin, end, x3::with<flag_tag>(flag)[parser::start], x3::space, _contents);
        if (flag)
            trim_contents(_contents);
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
