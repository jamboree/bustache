{{ bustache }}
========

C++1y implementation of [{{ mustache }}](http://mustache.github.io/)

## Dependency
* Boost
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

### Basics
{{ mustache }} is a template language for text-replacing.
When it comes to formatting, there are 2 essential things -- *Format* and *Data*.
In {{ bustache }}, we represent the Format as a `bustache::format` object, and `bustache::object` for Data.
Techincally, you can use your custom Data type, but then you have to write the formatting logic yourself.

### Data Model
It's basically the JSON Data Model represented in C++. 

__Header__: `#include <bustache/model.hpp>`

__Synopsis__

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

### Format Object
*TODO*

## Similar Projects
* [plustache](https://github.com/mrtazz/plustache)
* [Boostache](https://github.com/JeffGarland/liaw2014)
