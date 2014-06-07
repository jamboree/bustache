{{ bustache }}
========

C++1y implementation of [{{ mustache }}](http://mustache.github.io/)

## Dependencies
* [Boost](http://www.boost.org/)
* https://github.com/jamboree/spirit

## Supported Features
* Variables
* Sections
* Inverted Sections
* Comments
* Partials
* Set Delimiter
* HTML escaping *(configurable)*

## Unsupported Features
* Lambdas

## Basics
{{ mustache }} is a template language for text-replacing.
When it comes to formatting, there are 2 essential things -- *Format* and *Data*.
In {{ bustache }}, we represent the Format as a `bustache::format` object, and `bustache::object` for Data.
Techincally, you can use your custom Data type, but then you have to write the formatting logic yourself.

### Quick Example
```c++
bustache::format format{"{{mustache}} templating"};
bustache::object data{{"mustache", std::string("bustache")}};
std::cout << format(data); // should print "bustache templating"
```

## Manual

### Data Model
It's basically the JSON Data Model represented in C++. 

#### Header
`#include <bustache/model.hpp>`

#### Synopsis
```c++
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

struct array = std::vector<value>;
struct object = std::unordered_map<std::string, value>;
```
### Format Object
`bustache::format` parses in-memory string into AST.
It's worth noting that `bustache::format` *never* fails, if the input is ill-formed, `bustache::format` just treats them as plain-text.

#### Header
`#include <bustache/format.hpp>`

#### Synopsis
*Constructors*
```c++
// not ownig text
format(char const* begin, char const* end); // [1]

// not ownig text
template <std::size_t N>
format(char const (&source)[N]); // [2]

// not ownig text
template <typename Source>
explicit format(Source const& source); // [3]

// ownig text
template <typename Source>
explicit format(Source const&& source); // [4]

```
* `Source` is an object that represents continous memory, like `std::string`, `std::vector<char>` or `boost::iostreams::mapped_file_source` that provides access to raw memory through `source.data()` and `source.size()`.
* Version 2 allows implicit conversion from literal.
* Version 1~3 doesn't hold the text, you must ensure the memory referenced is valid and not modified at the use of the format object.
* Version 4 copies the necessary text into its internal buffer, so there's no lifetime issue.

*Manipulator*
```c++
template <typename Object>
manipulator<Object, no_context>
operator()(Object const& data, option_type flag = normal) const;

template <typename Object, typename Context>
manipulator<Object, Context>
operator()(Object const& data, Context const& context, option_type flag = normal) const;
```
* `Context` is any associative container `Map<std::string, bustache::format>`, which is referenced by Partials.
* `option_type` provides 2 options: `normal` and `escape_html`, if `normal` is chosen, there's no difference between `{{Tag}}` and `{{{Tag}}}`, the text won't be escaped in both cases.

### Stream-based Output
This is the most common usage.

#### Synopsis
```c++
template <typename CharT, typename Traits, typename Context>
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& out, manipulator<object, Context> const& manip);
```
#### Example
```c++
// open the template file 
boost::iostreams::mapped_file_source file(...);
// create format from source
bustache::format format(file);
// create the data we want to output
bustache::object data{...};
// create the context for Partials
std::unordered_map<std::string, bustache::format> context{...};
// output the result
std::cout << format(data, context, bustache::escape_html);
```

### Generate API
`generate` can be used for customized output.
In fact, the stream-based output is built on `generate`.

```c++
template <typename Sink>
void generate
(
    format const& fmt, object const& data, Sink const& sink
  , option_type flag = normal
);

template <typename Context, typename Sink>
void generate
(
    format const& fmt, object const& data, Context const& context
  , Sink const& sink, option_type flag = normal
);
```
`Sink` is a polymorphic functor that handles:
```c++
void operator()(char const* it, char const* end) const;
void operator()(bool data) const;
void operator()(int data) const;
void operator()(double data) const;
```
You don't have to worry about HTML-escaping here, it's handled within `generate` depending on the option.

## Inconformance
* We don't treat `{{{` and `}}}` as special tokens, instead, they're `{{` followed by `{` and `}}` preceded by `}`.
That means `{{ { Tag } }}` is also valid. If you change the Delimiter to, say, `[` and `]`, then `[{Tag}]` is valid as well.


## License

    Copyright (c) 2014 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
    
## Similar Projects
* [plustache](https://github.com/mrtazz/plustache)
* [Boostache](https://github.com/JeffGarland/liaw2014)
