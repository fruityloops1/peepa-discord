#pragma once

#include <cstdio>
#include <string>

std::string readStringFromFile(const std::string& filename);

template <typename... Args>
std::string format(const char* fmt, Args... args)
{
    int size = snprintf(nullptr, 0, fmt, args...);
    std::string buf(size+1, ' ');
    snprintf(buf.data(), size+1, fmt, args...);
    return buf;
}
