/*//////////////////////////////////////////////////////////////////////////////
    Copyright (c) 2014 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////*/
#ifndef BUSTACHE_MODEL_HPP_INCLUDED
#define BUSTACHE_MODEL_HPP_INCLUDED

#include "format.hpp"
#include <boost/io/ios_state.hpp>
#include <boost/variant.hpp>
#include <vector>
#include <unordered_map>

namespace bustache
{
    struct array;
    struct object;
    
    using value =
        boost::variant
        <
            std::nullptr_t
          , bool
          , int
          , double
          , std::string
          , boost::recursive_wrapper<array>
          , boost::recursive_wrapper<object>
        >;
    
    struct array : std::vector<value>
    {
        using vector::vector;
        using vector::operator=;
    };
    
    struct object : std::unordered_map<std::string, value>
    {
        using unordered_map::unordered_map;
        using unordered_map::operator=;
    };
}

namespace bustache { namespace detail
{
    template <typename Sink>
    struct value_printer
    {
        typedef void result_type;
        
        Sink const& sink;
        bool const escaping;

        void operator()(std::nullptr_t) const {}
        
        template <typename T>
        void operator()(T data) const
        {
            sink(data);
        }

        void operator()(std::string const& data) const
        {
            auto it = data.data(), end = it + data.size();
            if (escaping)
                escape_html(it, end);
            else
                sink(it, end);
        }
        
        void operator()(array const& data) const
        {
            auto it = data.begin(), end = data.end();
            if (it != end)
            {
                boost::apply_visitor(*this, *it);
                while (++it != end)
                {
                    literal(",");
                    boost::apply_visitor(*this, *it);
                }
            }
        }

        void operator()(object const&) const
        {
            literal("[Object]");
        }

        void escape_html(char const* it, char const* end) const
        {
            char const* last = it;
            while (it != end)
            {
                switch (*it)
                {
                case '&': sink(last, it); literal("&amp;"); break;
                case '<': sink(last, it); literal("&lt;"); break;
                case '>': sink(last, it); literal("&gt;"); break;
                case '\\': sink(last, it); literal("&#92;"); break;
                case '"': sink(last, it); literal("&quot;"); break;
                default:  ++it; continue;
                }
                last = ++it;
            }
            sink(last, it);
        }

        template <std::size_t N>
        void literal(char const (&str)[N]) const
        {
            sink(str, str + (N - 1));
        }
    };

    template <typename Sink, typename Context>
    struct content_visitor
    {
        typedef void result_type;

        content_visitor const* const parent;
        object const& data;
        Sink const& sink;
        Context const& context;
        bool const escaping;

        void operator()(ast::text const& text) const
        {
            sink(text.begin(), text.end());
        }
        
        void operator()(ast::variable const& variable) const
        {
            auto it = data.find(variable.id);
            if (it != data.end())
            {
                value_printer<Sink> printer{sink, escaping && !variable.tag};
                boost::apply_visitor(printer, it->second);
            }
            else if (parent)
                parent->operator()(variable);
        }
        
        void operator()(ast::section const& section) const
        {
            auto it = data.find(section.id);
            bool inverted = section.tag == '^';
            if (it != data.end())
            {
                struct value_extractor
                {
                    typedef bool result_type;
                    
                    content_visitor const& parent;
                    ast::content_list const& contents;
                    bool const inverted;
    
                    bool operator()(object const& data) const
                    {
                        if (!inverted)
                        {
                            content_visitor visitor
                            {
                                &parent, data, parent.sink
                              , parent.context, parent.escaping
                            };
                            for (auto const& content : contents)
                                boost::apply_visitor(visitor, content);
                        }
                        return false;
                    }
                    
                    bool operator()(array const& data) const
                    {
                        for (auto const& val : data)
                            boost::apply_visitor(*this, val);
                        return data.empty() && inverted;
                    }
    
                    bool operator()(bool data) const
                    {
                        return data ^ inverted;
                    }
    
                    bool operator()(std::string const& data) const
                    {
                        return !data.empty() ^ inverted;
                    }
                    
                    bool operator()(std::nullptr_t) const
                    {
                        return inverted;
                    }
                } extractor{*this, section.contents, inverted};
                if (!boost::apply_visitor(extractor, it->second))
                    return;
            }
            else if (!inverted)
                return;
                
            for (auto const& content : section.contents)
                boost::apply_visitor(*this, content);
        }
        
        void operator()(ast::partial const& partial) const
        {
            auto it = context.find(partial);
            if (it != context.end())
            {
                for (auto const& content : it->second.contents())
                    boost::apply_visitor(*this, content);
            }
        }
        
        void operator()(ast::comment) const {} // never called
    };

    template <typename CharT, typename Traits>
    struct ostream_sink
    {
        std::basic_ostream<CharT, Traits>& out;
        
        void operator()(char const* it, char const* end) const
        {
            out.write(it, end - it);
        }

        template <typename T>
        void operator()(T data) const
        {
            out << data;
        }
    };
}}

namespace bustache
{
    template <typename Sink>
    void generate
    (
        format const& fmt, object const& data, Sink const& sink
      , option_type flag = normal
    )
    {
        generate(fmt, data, no_context::dummy(), sink, flag);
    }
    
    template <typename Context, typename Sink>
    void generate
    (
        format const& fmt, object const& data, Context const& context
      , Sink const& sink, option_type flag = normal
    )
    {
        detail::content_visitor<Sink, Context> visitor
        {
            nullptr, data, sink, context, flag
        };
        for (auto const& content : fmt.contents())
            boost::apply_visitor(visitor, content);
    }
    
    template <typename CharT, typename Traits, typename Context>
    std::basic_ostream<CharT, Traits>&
    operator<<(std::basic_ostream<CharT, Traits>& out, manipulator<object, Context> const& manip)
    {
        boost::io::ios_flags_saver iosate(out);
        out << std::boolalpha;
        detail::ostream_sink<CharT, Traits> sink{out};
        generate(manip.fmt, manip.data, manip.context, sink, manip.flag);
        return out;
    }
}

#endif
