#include <iostream>
#include <bustache/model.hpp>

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

int main()
{
    using bustache::object;
    using bustache::array;
    using namespace bustache::literals;

    boost::unordered_map<std::string, bustache::format> context
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
        {"count", [&n] { return ++n; }},
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

    try
    {
        bustache::format format(tmp);
        std::cout << "-----------------------\n";
        std::cout << format(data, context, bustache::escape_html) << "\n";
        std::cout << "-----------------------\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what();
    }
}