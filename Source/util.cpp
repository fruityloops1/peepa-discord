#include "util.h"
#include <cstdlib>
#include <fstream>

std::string readStringFromFile(const std::string& filename) {
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
