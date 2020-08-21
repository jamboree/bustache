#include <benchmark/benchmark.h>
#include <bustache/render/string.hpp>
#include "model.hpp"
#include <mstch/mstch.hpp>
#include <mustache.hpp>

static char tmp[] =
R"(<h1>{{header}}</h1>
{{#bug}}
{{/bug}}

{{# items}}
  {{#first}}
    <li><strong>{{name}}</strong></li>
  {{/first}}
  {{#link}}
    <li><a {{>href}}>{{name}}</a></li>
  {{/link}}
{{ /items}}

{{#empty}}
  <p>The list is empty.</p>
{{/ empty }}

{{=[ ]=}}

[#array]([.])[/array]

[#items]
[count]->[count]->[count]
[/items]

[a.b.c] == [#a][#b][c][/b][/a]

<div class="comments">
    <h3>[header]</h3>
    <ul>
        [#comments]
        <li class="comment">
            <h5>[name]</h5>
            <p>[body]</p>
        </li>
        <!--[count]-->
        [/comments]
    </ul>
</div>)";

static void bustache_usage(benchmark::State& state)
{
    using namespace bustache;
    using namespace test;

    test::context context
    {
        {"href", "href=\"{{url}}\""_fmt}
    };

    int n = 0;
    object data
    {
        {"header", "Colors"},
        {"items",
            array
            {
                object
                {
                    {"name", "red"},
                    {"first", true},
                    {"url", "#Red"}
                },
                object
                {
                    {"name", "green"},
                    {"link", true},
                    {"url", "#Green"}
                },
                object
                {
                    {"name", "blue"},
                    {"link", true},
                    {"url", "#Blue"}
                }
            }
        },
        {"empty", false},
        {"count", [&n](...) { return ++n; }},
        {"array", array{1, 2, 3}},
        {"a", object{{"b", object{{"c", true}}}}},
        {"comments",
            array
            {
                object
                {
                    {"name", "Joe"},
                    {"body", "<html> should be escaped"}
                },
                object
                {
                    {"name", "Sam"},
                    {"body", "{{mustache}} can be seen"}
                },
                object
                {
                    {"name", "New"},
                    {"body", "break\nup"}
                }
            }
        }
    };

    format fmt(tmp);

    for (auto _ : state)
    {
        n = 0;
        to_string(fmt(data).context(context).escape(escape_html));
    }
}

static void mstch_usage(benchmark::State& state)
{
    using namespace mstch;
    using namespace std::string_literals;

    std::map<std::string, std::string> context
    {
        {"href", "href=\"{{url}}\""}
    };

    int n = 0;
    map data
    {
        {"header", "Colors"s},
        {"items",
            array
            {
                map
                {
                    {"name", "red"s},
                    {"first", true},
                    {"url", "#Red"s}
                },
                map
                {
                    {"name", "green"s},
                    {"link", true},
                    {"url", "#Green"s}
                },
                map
                {
                    {"name", "blue"s},
                    {"link", true},
                    {"url", "#Blue"s}
                }
            }
        },
        {"empty", false},
        {"count", lambda{[&n]() -> node { return ++n; }}},
        {"array", array{1, 2, 3}},
        {"a", map{{"b", map{{"c", true}}}}},
        {"comments",
            array
            {
                map
                {
                    {"name", "Joe"s},
                    {"body", "<html> should be escaped"s}
                },
                map
                {
                    {"name", "Sam"s},
                    {"body", "{{mustache}} can be seen"s}
                },
                map
                {
                    {"name", "New"s},
                    {"body", "break\nup"s}
                }
            }
        }
    };

    for (auto _ : state)
    {
        n = 0;
        render(tmp, data, context);
    }
}

static void kainjow_usage(benchmark::State& state)
{
    using namespace kainjow::mustache;

    int n = 0;
    object dat
    {
        {"header", "Colors"},
        {"items",
            list
            {
                object
                {
                    {"name", "red"},
                    {"first", true},
                    {"url", "#Red"}
                },
                object
                {
                    {"name", "green"},
                    {"link", true},
                    {"url", "#Green"}
                },
                object
                {
                    {"name", "blue"},
                    {"link", true},
                    {"url", "#Blue"}
                }
            }
        },
        {"empty", false},
        {"count", lambda{[&n](const std::string&) { return std::to_string(++n); }}},
        // Kainjow cannot print numbers and boolean, can only use strings here.
        {"array", list{"1", "2", "3"}},
        {"a", object{{"b", object{{"c", "true"}}}}},
        {"comments",
            list
            {
                object
                {
                    {"name", "Joe"},
                    {"body", "<html> should be escaped"}
                },
                object
                {
                    {"name", "Sam"},
                    {"body", "{{mustache}} can be seen"}
                },
                object
                {
                    {"name", "New"},
                    {"body", "break\nup"}
                }
            }
        },
        // In Kainjow, partail belongs to data.
        {"href", partial{[]() { return "href=\"{{url}}\""; }}}
    };
    
    mustache fmt(tmp);

    for (auto _ : state)
    {
        n = 0;
        fmt.render(dat);
    }
}

BENCHMARK(bustache_usage);
BENCHMARK(mstch_usage);
BENCHMARK(kainjow_usage);

BENCHMARK_MAIN();