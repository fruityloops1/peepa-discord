#pragma once

#include <cstdio>
#include <string>

std::string readStringFromFile(const std::string& filename);
bool isGarbageSymbol(const std::string& symbol);

template <typename... Args>
std::string format(const char* fmt, Args... args)
{
    int size = snprintf(nullptr, 0, fmt, args...);
    std::string buf(size + 1, ' ');
    snprintf(buf.data(), size + 1, fmt, args...);
    return buf;
}

std::string truncateString(const std::string& str, int size = 2048);
std::string generateRandomString(int n);
