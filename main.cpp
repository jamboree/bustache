#include <iostream>
#include <boost/iostreams/device/mapped_file.hpp>
#include <bustache/value.hpp>

int main(int argc, char** argv) {
    namespace bu = bustache;
    bu::object obj
    {
        {"header", std::string("Colors")}
      , {"items", bu::array{
            bu::object
            {
                {"name", std::string("red")}
              , {"first", true}
              , {"url", std::string("#Red")}
            }
          , bu::object
            {
                {"name", std::string("green")}
              , {"link", true}
              , {"url", std::string("#Green")}
            }
          , bu::object
            {
                {"name", std::string("blue")}
              , {"link", true}
              , {"url", std::string("#Blue")}
            }
        }}
      , {"empty", false}
    };
    std::string fname;
    while (std::getline(std::cin, fname))
    {
        boost::iostreams::mapped_file_source file(fname);
        bu::format fmt(file.begin(), file.end());
        std::cout << "-----------------------\n";
        std::cout << fmt(obj);
        std::cout << "-----------------------\n";
    }

	return 0;
}
