#include <iostream>
#include <boost/iostreams/device/mapped_file.hpp>
#include <bustache/value.hpp>

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
    std::string fname;
    while (std::getline(std::cin, fname))
    {
        boost::iostreams::mapped_file_source file(fname);
        bustache::format format(file.begin(), file.end());
        std::cout << "-----------------------\n";
        std::cout << format(data) << std::endl;
        std::cout << "-----------------------\n";
    }

    return 0;
}
