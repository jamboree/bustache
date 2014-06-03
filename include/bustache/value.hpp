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
    };
    
    struct object : std::unordered_map<std::string, value>
    {
        using unordered_map::unordered_map;
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

    template <typename OStream>
    struct content_visitor
    {
        typedef void result_type;
        
        object const& obj;
        OStream& out;
        
        void operator()(ast::text const& text) const
        {
            out << text;
        }
        
        void operator()(ast::id const& id) const
        {
            auto it = obj.find(id);
            if (it != obj.end())
                boost::apply_visitor(value_printer<OStream>{out}, it->second);
        }
        
        void operator()(ast::block const& block) const
        {
            auto it = obj.find(block.id);
            if (it != obj.end())
            {
                struct value_extractor
                {
                    typedef void result_type;
                    
                    object const& parent;
                    ast::block const& block;
                    OStream& out;

                    void operator()(object const& data) const
                    {
                        content_visitor visitor{data, out};
                        for (auto const& content : block.contents)
                            boost::apply_visitor(visitor, content);
                    }
                    
                    void operator()(array const& data) const
                    {
                        for (auto const& val : data)
                            boost::apply_visitor(*this, val);
                    }

                    void operator()(bool data) const
                    {
                        if (data)
                            operator()(parent);
                    }

                    void operator()(std::string const& data) const
                    {
                        if (!data.empty())
                            operator()(parent);
                    }
                    
                    void operator()(boost::blank) const {}
                };
                boost::apply_visitor(value_extractor{obj, block, out}, it->second);
            }
        }
    };

    template <typename CharT, typename Traits>
    std::basic_ostream<CharT, Traits>&
    operator<<(std::basic_ostream<CharT, Traits>& out, format::manip<object> const& fmt)
    {
        boost::io::ios_flags_saver iosate(out);
        out << std::boolalpha;
        content_visitor<std::basic_ostream<CharT, Traits>> visitor{fmt.data, out};
        for (auto const& content : fmt.contents)
            boost::apply_visitor(visitor, content);
        return out;
    }
}

#endif
