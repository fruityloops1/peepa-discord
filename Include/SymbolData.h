#pragma once

#include "snowflake.h"
#include <bits/types/time_t.h>
#include <cstdint>
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
    std::vector<Symbol> symbols;
    std::vector<Comment> comments;

    void save(const std::string& file);
    void load(const std::string& file);
};
