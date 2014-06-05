#include <iostream>
#include <unordered_map>
#include <boost/iostreams/device/mapped_file.hpp>
#include <bustache/model.hpp>

int main(int argc, char** argv)
{
    using bustache::object;
    using bustache::array;
    
    object data
    {
        {"header", std::string("Colors")}
      , {"items",
            array
            {
                object
                {
                    {"name", std::string("red")}
                  , {"first", true}
                  , {"url", std::string("#Red")}
                }
              , object
                {
                    {"name", std::string("green")}
                  , {"link", true}
                  , {"url", std::string("#Green")}
                }
              , object
                {
                    {"name", std::string("blue")}
                  , {"link", true}
                  , {"url", std::string("#Blue")}
                }
            }
        }
      , {"empty", false}
    };

    std::unordered_map<std::string, bustache::format> context
    {
        {"href", "href=\"{{url}}\""}
    };
        
    std::string fname;
    while (std::getline(std::cin, fname))
    {
        boost::iostreams::mapped_file_source file(fname);
        bustache::format format(file);
        std::cout << "-----------------------\n";
        std::cout << format(data, context) << std::endl;
        std::cout << "-----------------------\n";
    }

    return 0;
}
