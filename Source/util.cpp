#include "util.h"
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>

bool isGarbageSymbol(const std::string& symbol)
{
    for (const char* str : {"FUN_", "LAB_", "sub_", "nullsub"})
    {
        if (symbol.find(str) != std::string::npos)
            return true;
    }
    return false;
}

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

std::string truncateString(const std::string& str, int size)
{
    if (str.length() > size - 1) {
        std::string truncatedString = str.substr(0, size - 4) + "...";
        return truncatedString;
    } else {
        return str;
    }
}

std::string generateRandomString(int n)
{
    std::string result;
    static const char stuff[] =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890#.,-";

    int alphabetSize = sizeof(stuff) - 1;

    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    for (int i = 0; i < n; ++i) {
        int randomIndex = std::rand() % alphabetSize;
        result += stuff[randomIndex];
    }

    return result;
}
