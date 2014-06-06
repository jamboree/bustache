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

    template <typename OStream>
    struct value_printer
    {
        typedef void result_type;
        
        OStream& out;
        bool const escaping;

        void operator()(std::nullptr_t) const {}
        
        template <typename T>
        void operator()(T const& data) const
        {
            out << data;
        }

        void operator()(std::string const& data) const
        {
            if (escaping)
                escape_html(data.data(), data.data() + data.size());
            else
                out << data;
        }
        
        void operator()(array const& data) const
        {
            auto it = data.begin(), end = data.end();
            if (it != end)
            {
                boost::apply_visitor(*this, *it);
                while (++it != end)
                {
                    out << ',';
                    boost::apply_visitor(*this, *it);
                }
            }
        }

        void operator()(object const&) const
        {
            out << "[Object]";
        }
        
        template <std::size_t N>
        void catenate(char const* it, char const* end, char const (&str)[N]) const
        {
            out.write(it, end - it);
            out.write(str, N - 1);
        }
    
        void escape_html(char const* it, char const* end) const
        {
            char const* last = it;
            while (it != end)
            {
                switch (*it)
                {
                case '&': catenate(last, it, "&amp;"); break;
                case '<': catenate(last, it, "&lt;"); break;
                case '>': catenate(last, it, "&gt;"); break;
                case '\\': catenate(last, it, "&#92;"); break;
                case '"': catenate(last, it, "&quot;"); break;
                default:  ++it; continue;
                }
                last = ++it;
            }
            out.write(last, it - last);
        }
    };

    template <typename OStream, typename Context>
    struct content_visitor
    {
        typedef void result_type;

        content_visitor const* const parent;
        object const& data;
        OStream& out;
        Context const& context;
        bool const escaping;

        void operator()(ast::text const& text) const
        {
            out << text;
        }
        
        void operator()(ast::variable const& variable) const
        {
            auto it = data.find(variable.id);
            if (it != data.end())
            {
                value_printer<OStream> printer{out, escaping && !variable.tag};
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
                                &parent, data, parent.out
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

    template <typename CharT, typename Traits, typename Context>
    std::basic_ostream<CharT, Traits>&
    operator<<(std::basic_ostream<CharT, Traits>& out, manipulator<object, Context> const& manip)
    {
        boost::io::ios_flags_saver iosate(out);
        out << std::boolalpha;
        content_visitor<std::basic_ostream<CharT, Traits>, Context> visitor
        {
            nullptr, manip.data, out, manip.context, manip.flag
        };
        for (auto const& content : manip.contents)
            boost::apply_visitor(visitor, content);
        return out;
    }
}

#endif
