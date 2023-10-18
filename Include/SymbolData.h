#pragma once

#include "snowflake.h"
#include <bits/types/time_t.h>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

struct Symbol {
    uint32_t addr;
    std::string name;
    time_t lastUpdated;
    dpp::snowflake updatedUser;
};

struct Comment {
    uint32_t addr;
    std::string comment;
    time_t added;
    dpp::snowflake user;
};

struct SymbolDB {
    std::map<uint32_t, Symbol> symbols;
    std::vector<Comment> comments;

    void save(const std::string& file);
    void load(const std::string& file);
    void loadFromStream(std::istream& data, const std::string& filename);
    std::string saveSymbols();
    std::string saveComments();
};
