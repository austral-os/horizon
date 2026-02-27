#include <horizon/IconThemeLookup.hpp>
#include <iostream>
#include <string>

using namespace horizon;

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " <icon_name> [size]" << std::endl;
        return 1;
    }

    std::string name = argv[1];
    int size = (argc > 2) ? std::stoi(argv[2]) : 24;

    std::cout << "Looking up icon: " << name << " at size: " << size << std::endl;

    std::string path = IconThemeLookup::find_icon(name, size);

    if (path.empty())
    {
        std::cout << "Icon not found!" << std::endl;
    }
    else
    {
        std::cout << "Found: " << path << std::endl;
    }

    return 0;
}
