#include "util.h"
#include <cstdlib>
#include <fstream>

std::string readStringFromFile(const std::string& filename)
{
    std::ifstream file(filename);
    std::string content;

    if (file.is_open()) {
        std::getline(file, content);
        file.close();
    } else {
        abort();
    }

    return content;
}

std::string truncateString(const std::string& str)
{
    if (str.length() > 2047) {
        std::string truncatedString = str.substr(0, 2044) + "...";
        return truncatedString;
    } else {
        return str;
    }
}
