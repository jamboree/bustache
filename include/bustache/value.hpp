/*//////////////////////////////////////////////////////////////////////////////
    Copyright (c) 2014 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////*/
#ifndef BUSTACHE_VALUE_HPP_INCLUDED
#define BUSTACHE_VALUE_HPP_INCLUDED

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
            boost::blank
          , bool
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
    
    struct unused_type
    {
        template <typename T>
        unused_type(T const&) {}
    };
    
    template <typename OStream>
    struct value_printer
    {
        typedef void result_type;
        
        OStream& out;

        void operator()(boost::blank) const
        {}
        
        template <typename T>
        void operator()(T const& val) const
        {
            out << val;
        }

        void operator()(array const& data) const
        {
            multi(data, '[', ']', boost::apply_visitor(*this));
        }

        void operator()(object const& data) const
        {
            multi(data, '{', '}', [this](object::value_type const& pair)
            {
                out << pair.first << ':';
                boost::apply_visitor(*this, pair.second);
            });
        }
        
        template <typename T, typename F>
        void multi(T const& data, char left, char right, F const& f) const
        {
            out << left;
            auto it = data.begin(), end = data.end();
            if (it != end)
            {
                f(*it);
                while (++it != end)
                {
                    out << ',';
                    f(*it);
                }
            }
            out << right;
        }
    };

    template <typename OStream, typename Context>
    struct content_visitor
    {
        typedef void result_type;
        
        object const& obj;
        Context const& context;
        OStream& out;
        
        void operator()(ast::text const& text) const
        {
            out << text;
        }
        
        void operator()(ast::variable const& variable) const
        {
            auto it = obj.find(variable);
            if (it != obj.end())
                boost::apply_visitor(value_printer<OStream>{out}, it->second);
        }
        
        void operator()(ast::section const& section) const
        {
            auto it = obj.find(section.id);
            bool inverted = section.tag == '^';
            if (it != obj.end())
            {
                struct value_extractor
                {
                    typedef bool result_type;
                    
                    object const& parent;
                    Context const& context;
                    ast::section const& section;
                    bool inverted;
                    OStream& out;
    
                    bool operator()(object const& data) const
                    {
                        if (!inverted)
                        {
                            content_visitor visitor{data, context, out};
                            for (auto const& content : section.contents)
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
                    
                    bool operator()(boost::blank) const
                    {
                        return inverted;
                    }
                } extractor{obj, context, section, inverted, out};
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
                content_visitor visitor{obj, context, out};
                for (auto const& content : it->second.contents())
                    boost::apply_visitor(visitor, content);
            }
        }
    };

    template <typename CharT, typename Traits, typename Context>
    std::basic_ostream<CharT, Traits>&
    operator<<(std::basic_ostream<CharT, Traits>& out, manipulator<object, Context> const& manip)
    {
        boost::io::ios_flags_saver iosate(out);
        out << std::boolalpha;
        content_visitor<std::basic_ostream<CharT, Traits>, Context>
            visitor{manip.data, manip.context, out};
        for (auto const& content : manip.contents)
            boost::apply_visitor(visitor, content);
        return out;
    }
}

#endif
