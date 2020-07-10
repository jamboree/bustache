{{ bustache }} [![Try it online][badge.wandbox]](https://wandbox.org/permlink/HC4GG9QxCw6dsygF)
========

C++11 implementation of [{{ mustache }}](http://mustache.github.io/), compliant with [spec](https://github.com/mustache/spec) v1.1.3.

## Dependencies
* [Boost](http://www.boost.org/) - for `unordered_map`, etc

### Optional Dependencies
* [Google.Benchmark](https://github.com/google/benchmark) - for benchmark
* [Catch](https://github.com/philsquared/Catch) - for test

## Supported Features
* Variables
* Sections
* Inverted Sections
* Comments
* Partials
* Set Delimiter
* Lambdas
* HTML escaping *(configurable)*
* Template inheritance *(extension)*
* Customizable behavior on unresolved key
* User-defined object and list

## Basics
{{ mustache }} is a template language for text-replacing.
When it comes to formatting, there are 2 essential things -- _Format_ and _Data_.
{{ mustache }} also allows an extra lookup-context for _Partials_.
In {{ bustache }}, we represent the _Format_ as a `bustache::format` object, and `bustache::object` for _Data_, and anything that provides interface that is compatible with `Map<std::string, bustache::format>` can be used for _Partials_.
The _Format_ is orthogonal to the _Data_, so technically you can use your custom _Data_ type with `bustache::format`, but then you have to write the formatting logic yourself.

### Quick Example
```c++
bustache::format format{"{{mustache}} templating"};
bustache::object data{{"mustache", "bustache"}};
std::cout << format(data); // should print "bustache templating"
```

## Manual

### Data Model
It's basically the JSON Data Model represented in C++, with some extensions.

#### Header
`#include <bustache/model.hpp>`

#### Synopsis
```c++
using array = std::vector<value>;
using object = boost::unordered_map<std::string, value>;
using lambda0v = std::function<value()>;
using lambda0f = std::function<format()>;
using lambda1v = std::function<value(ast::content_list const&)>;
using lambda1f = std::function<format(ast::content_list const&)>;

// Non-owning UDT views.
class object_view;
class list_view;

class value =
    variant
    <
        std::nullptr_t
      , bool
      , int
      , double
      , std::string
      , array
      , lambda0v
      , lambda0f
      , lambda1v
      , lambda1f
      , object
      , object_view
      , list_view
    >;
```
### Format Object
`bustache::format` parses in-memory string into AST.

#### Header
`#include <bustache/format.hpp>`

#### Synopsis
*Constructors*
```c++
format(char const* begin, char const* end); // [1]

template<std::size_t N>
explicit format(char const (&source)[N]); // [2]

template<class Source>
explicit format(Source const& source); // [3]

explicit format(ast::content_list contents, bool copytext = true); // [4]
```
* `Source` is an object that represents continous memory, like `std::string`, `std::vector<char>` or `boost::iostreams::mapped_file_source` that provides access to raw memory through `source.data()` and `source.size()`.
* Version 2 allows implicit conversion from literal.
* Version 1~3 doesn't hold the text, you must ensure the memory referenced is valid and not modified at the use of the format object.
* Version 4 takes a `ast::content_list`, if `copytext == true` the text will be copied into the internal buffer.

*Manipulator*
```c++
template <typename T>
manipulator<T, no_context>
operator()(T const& data, option_type flag = normal) const;

template <typename T, typename Context>
manipulator<T, Context>
operator()(T const& data, Context const& context, option_type flag = normal) const;
```
* `Context` is the lookup-context used by _Partials_.
You can implement `bustache::context_trait<T>` for your custom context:
```c++
template<>
struct context_trait<T>
{
    static format const* get(T const& self, std::string const& key);
};
```
`context_trait` has default implementation for `Map<std::string, bustache::format>`, where `Map` has interface like `map` or `unordered_map`.
* `option_type` provides 2 options: `normal` and `escape_html`, if `normal` is chosen, there's no difference between `{{Tag}}` and `{{{Tag}}}`, the text won't be escaped in both cases.

### Stream-based Output
Output directly to the `std::basic_ostream`.

#### Synopsis
```c++
// in <bustache/model.hpp>
template<class CharT, class Traits, class T, class Context,
    std::enable_if_t<std::is_constructible<value_view, T>::value, bool> = true>
inline std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& out, manipulator<T, Context> const& manip)
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
Note that you can output anything that constitutes `bustache::value`, not just `bustache::object`.

### String Output
Generate a `std::string` from a `manipulator`.

#### Synopsis
```c++
// in <bustache/model.hpp>
template<class T, class Context,
    std::enable_if_t<std::is_constructible<value_view, T>::value, bool> = true>
inline std::string to_string(manipulator<T, Context> const& manip)
```
#### Example
```c++
bustache::format format(...);
std::string txt = to_string(format(data, context, bustache::escape_html));
```

### Generate API
`generate` can be used for customized output.

#### Header
`#include <bustache/generate.hpp>`

```c++
template<class Sink, class UnresolvedHandler = default_unresolved_handler>
inline void generate
(
    Sink& sink, format const& fmt, value_view const& data,
    option_type flag = normal, UnresolvedHandler&& f = {}
)
{
    generate(sink, fmt, data, no_context::dummy(), flag, std::forward<UnresolvedHandler>(f));
}

template<class Sink, class Context, class UnresolvedHandler = default_unresolved_handler>
void generate
(
    Sink& sink, format const& fmt, value_view const& data,
    Context const& context, option_type flag = normal, UnresolvedHandler&& f = {}
)
```
`Sink` is a polymorphic functor that handles:
```c++
void operator()(char const* it, char const* end);
void operator()(bool data);
void operator()(int data);
void operator()(double data);
```
You don't have to deal with HTML-escaping yourself, it's handled within `generate` depending on the option.

`UnresolvedHandler` is a callable object that has the signature:
```c++
value(std::string const& key);
```
The `key` parameter is the unresolved key. The default handler just returns a null value.

### Predefined Generators
These are predefined output built on `generate`.

#### Header
* `#include <bustache/generate/ostream.hpp>`
* `#include <bustache/generate/string.hpp>`

```c++
template<class CharT, class Traits, class Context, class UnresolvedHandler = default_unresolved_handler>
void generate_ostream
(
    std::basic_ostream<CharT, Traits>& out, format const& fmt,
    value_view const& data, Context const& context,
    option_type flag, UnresolvedHandler&& f = {}
);

template<class String, class Context, class UnresolvedHandler = default_unresolved_handler>
void generate_string
(
    String& out, format const& fmt,
    value_view const& data, Context const& context,
    option_type flag, UnresolvedHandler&& f = {}
);
```

#### Note
The stream-based output and string output are built on these functions,
but `<bustache/model.hpp>` doesn't include these headers and only supports `char` output,
if you need other char-type support for stream/string output, you have to include these headers as well.


## Advanced Topics
### Lambdas
The lambdas in {{ bustache }} have 4 variants - they're production of 2 param-set x 2 return-type.
One param-set accepts no params, the other accepts a `bustache::ast::content_list const&`.
One return-type is `bustache::value`, the other is `bustache::format`.

Note that unlike other implementations, we pass a `bustache::ast::content_list` instead of a raw string.
A `content_list` is a parsed list of AST nodes, you can make a new `content_list` out of the old one and give it to a `bustache::format`.

### UDT
Sometimes it's infeasible or inefficent to transform the whole user data to the JSON-like data model.
Fortunately, {{ bustache }} allows users to use custom types in the data model, by implementing the required traits and using the corresponding views for the UDTs.
#### Custom Object
Implement `bustache::object_trait` for your object type `T`:
```c++
template<>
struct object_trait<T>
{
    static value_ptr get(T const& self, std::string const& key, value_holder& hold);
};
```
Use `bustache::object_view` to make a view to the object:
```c++
T data;
object_view(data);
```
#### Custom List
Implement `bustache::list_trait` for your list type `T`:
```c++
template<>
struct list_trait<T>
{
    static bool empty(T const& self);
    static std::uintptr_t begin_cursor(T const& self);
    static value_ptr next(T const& self, std::uintptr_t& state, value_holder& hold);
    static void end_cursor(T const& self, std::uintptr_t) noexcept {}
};
```
Use `bustache::list_view` to make a view to the list:
```c++
T data;
list_view(data);
```
#### Use of value_holder
Note that both `object_trait<T>::get` and `list_trait<T>::next` take a `bustache::value_holder&` and return a `bustache::value_ptr`.
The value that `value_ptr` refers to must outlive the pointer, and that's what `value_holder` offers - to hold the value.

To return a local value:
```c++
return hold(10);
```
To return an `object_view` to some data member:
```c++
return hold(object_view(self.obj));
```
To return a local object:
```c++
return hold.object(obj); // obj is copied.
```
To return a local list:
```c++
return hold.list(lst); // lst is copied.
```

It's not always necessary to use `value_holder`. If you know the data outlives the returned `value_ptr`, you can do something like:
```c++
return value_view(self.data).get_pointer();
```

### Error Handling
The constructor of `bustache::format` may throw `bustache::format_error` if the parsing fails.
```c++
class format_error : public std::runtime_error
{
public:
    explicit format_error(error_type err, std::ptrdiff_t position);

    error_type code() const noexcept;
    std::ptrdiff_t position() const noexcept;
};
```
`error_type` has these values:
* error_set_delim
* error_baddelim
* error_delim
* error_section
* error_badkey

You can also use `what()` for a descriptive text.

## Performance
Compare with 2 other libs - [mstch](https://github.com/no1msd/mstch) and [Kainjow.Mustache](https://github.com/kainjow/Mustache).
See [benchmark.cpp](test/benchmark.cpp). 

Sample run (VS2017 15.7.4, boost 1.67.0, 64-bit release build):
```
06/25/18 16:14:52
Run on (8 X 3392 MHz CPU s)
CPU Caches:
  L1 Data 32K (x4)
  L1 Instruction 32K (x4)
  L2 Unified 262K (x4)
  L3 Unified 8388K (x1)
------------------------------------------------------
Benchmark               Time           CPU Iterations
------------------------------------------------------
bustache_usage       6605 ns       6627 ns      89600
mstch_usage        106620 ns     106027 ns       5600
kainjow_usage       22828 ns      22949 ns      32000
```
Lower is better.

![benchmark](doc/benchmark.png?raw=true)

## License

    Copyright (c) 2014-2020 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

<!-- Links -->
[badge.Wandbox]: https://img.shields.io/badge/try%20it-online-green.svg
