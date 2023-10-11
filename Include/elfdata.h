#pragma once

#include "elfio/elfio.hpp"
#include <map>
#include <string>

void loadElfData();

struct Symbol {
    ELFIO::Elf64_Addr addr;
    ELFIO::Elf_Xword size;
    std::string name;
};

const std::vector<Symbol>& getSymbolsKiosk();
const std::vector<Symbol>& getSymbolsRelease();
const std::vector<Symbol>& getSymbolsWiiuEu();

std::string demangle(const std::string& input);
