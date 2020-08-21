{{ bustache }} <!-- [![Try it online][badge.wandbox]](https://wandbox.org/permlink/HC4GG9QxCw6dsygF) -->
========

C++20 implementation of [{{ mustache }}](http://mustache.github.io/), compliant with [spec](https://github.com/mustache/spec) v1.1.3.
No external dependencies required.

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

## Other Features
* Customizable behavior on unresolved variable
* Trait-based user-defined model

## Basics
{{ mustache }} is a template language for text-replacing.
When it comes to formatting, there are 2 essential things -- _Format_ and _Data_.
{{ mustache }} also allows an extra lookup-context for _Partials_.
In {{ bustache }}, we represent the _Format_ as a `bustache::format` object, and _Data_ and _Partials_ can be anything that implements the required traits.

### Quick Example
```c++
bustache::format format{"{{mustache}} templating"};
std::unordered_map<std::string, std::string> data{{"mustache", "bustache"}};
std::cout << format(data); // should print "bustache templating"
```

## Manual

### Data Model
{{ bustache }} doesn't required a fixed set of predefined data types to be used as data model. Instead, any type can be used as data model.
Most STL-compatible containers will work out-of-the-box, including the ones that you defined yourself!

#### Header
`#include <bustache/model.hpp>`

#### Model Traits
To meet the `Model` concept, you have to implement the traits:
```c++
template<>
struct bustache::impl_model<T>
{
    static constexpr model kind;
};
```
where model can be one of the following:
* `model::atom`
* `model::object`
* `model::list`

```c++
// Required by model::atom.
template<>
struct bustache::impl_test<T>
{
    static bool test(T const& self);
};

// Required by model::atom.
template<>
struct bustache::impl_print<T>
{
    static void print(T const& self, output_handler os, char const* fmt);
};

// Required by model::object.
template<>
struct bustache::impl_object<T>
{
    static void get(T const& self, std::string const& key, value_handler visit);
};

// Required by model::list.
template<>
struct bustache::impl_list<T>
{
    static bool empty(T const& self);
    static void iterate(T const& self, value_handler visit);
};
```
See [udt.cpp](test/udt.cpp) for more examples.

#### Compatible Trait
Some types cannot be categorized into a single model (e.g. `varaint`), to make it compatible, you can implement the trait:
```c++
template<>
struct bustache::impl_compatible<T>
{
    static value_ptr get_value_ptr(T const& self);
};
```
See [model.hpp](test/model.hpp) for example.

### Format Object
`bustache::format` parses in-memory string into AST.

#### Header
`#include <bustache/format.hpp>`

#### Synopsis
*Constructors*
```c++
explicit format(std::string_view source); // [1]
format(std::string_view source, bool copytext); // [2]
format(ast::content_list contents, bool copytext); // [3]
```
* Version 1 doesn't hold the text, you must ensure the source is valid and not modified during its use.
* Version 2~3, if `copytext == true` the text will be copied into the internal buffer.

*Manipulator*

A manipulator combines the format & data and allows you to specify some options.
```c++
template<class T>
manipulator</*unspecified*/> format::operator()(T const& data) const;

// Specify the context for partials.
template<class T>
manipulator</*unspecified*/> manipulator::context(T const&) const noexcept;

// Specify the escape action.
template<class T>
manipulator</*unspecified*/> manipulator::escape(T const&) const noexcept;
```

### Render API
`render` can be used for customized output.

#### Header
`#include <bustache/render.hpp>`

```c++
template<class Sink, Value T, class Escape = no_escape_t>
inline void render
(
    Sink const& os, format const& fmt, T const& data,
    context_handler context = no_context_t{}, Escape escape = {},
    unresolved_handler f = nullptr
);
```

#### Context Handler
The context for partials can be any callable that meets the signature:
```c++
(std::string const& key) -> format const*;
```

#### Unresolved Handler
The unresolved handler can be any callable that meets the signature:
```c++
(std::string const& key) -> value_ptr;
```

#### Sink (Output Handler)
The sink can be any callable that meets the signature:
```c++
(char const* data, std::size_t count) -> void;
```

#### Escape Action
The escape action can be any callable that meets the signature:
```c++
template<class OldSink>
(OldSink const& sink) -> NewSink;
```
There're 2 predefined actions: `no_escape` (default) and `escape_html`, if `no_escape` is chosen, there's no difference between `{{Tag}}` and `{{{Tag}}}`, the text won't be escaped in both cases.

### Stream-based Output
Output directly to the `std::basic_ostream`.

#### Header
`#include <bustache/render/ostream.hpp>`

#### Synopsis
```c++
template<class CharT, class Traits, Value T, class Escape = no_escape_t>
void render_ostream
(
    std::basic_ostream<CharT, Traits>& out, format const& fmt,
    T const& data, context_handler context = no_context_t{},
    Escape escape = {}, unresolved_handler f = nullptr
);

template<class CharT, class Traits, class... Opts>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& out, manipulator<Opts...> const& manip)
```

#### Example
```c++
// Create format from source.
bustache::format format(...);
// Create the data we want to output.
my_data data{...};
// Create the context for Partials.
my_context context{...};
// Output the result.
std::cout << format(data).context(context).escape(bustache::escape_html);
```

### String Output
Generate a `std::string`.

#### Header
`#include <bustache/render/string.hpp>`

#### Synopsis
```c++
template<class String, Value T, class Escape = no_escape_t>
void render_string
(
    String& out, format const& fmt,
    T const& data, context_handler context = no_context_t{},
    Escape escape = {}, unresolved_handler f = nullptr
);

template<class... Opts>
std::string to_string(manipulator<Opts...> const& manip);
```
#### Example
```c++
bustache::format format(...);
std::string txt = to_string(format(data).context(context).escape(bustache::escape_html));
```

## Advanced Topics
### Lambdas
The lambdas in {{ bustache }} accept signatures below:
* `(ast::content_list const* contents) -> format`
* `(ast::content_list const* contents) -> Value`

A `ast::content_list` is a parsed list of AST nodes, you can make a new `ast::content_list` out of the old one and give it to a `format`. Note that `contents` will be null if the lambda is used as variable.

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
Compare with 2 other libs - [mstch](https://github.com/no1msd/mstch/tree/0fde1cf94c26ede7fa267f4b64c0efe5da81a77a) and [Kainjow.Mustache](https://github.com/kainjow/Mustache/tree/a7eebc9bec92676c1931eddfff7637d7e819f2d2).
See [benchmark.cpp](test/benchmark.cpp). 

Sample run (VS2019 16.7.2, boost 1.73.0, 64-bit release build):
```
08/21/20 09:31:39
Running F:\code\Notation\x64\Release\Notation.exe
Run on (8 X 3600 MHz CPU s)
CPU Caches:
  L1 Data 32K (x8)
  L1 Instruction 32K (x8)
  L2 Unified 262K (x8)
  L3 Unified 12582K (x1)
---------------------------------------------------------
Benchmark               Time             CPU   Iterations
---------------------------------------------------------
bustache_usage       4366 ns         4395 ns       160000
mstch_usage         70794 ns        71498 ns         8960
kainjow_usage       25155 ns        25112 ns        28000
```
Lower is better.

![benchmark](doc/benchmark.png?raw=true)

## License

    Copyright (c) 2014-2020 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

<!-- Links -->
[badge.Wandbox]: https://img.shields.io/badge/try%20it-online-green.svg
