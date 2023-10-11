#include "elfdata.h"
#include "elfio/elf_types.hpp"
#include "elfio/elfio.hpp"
#include <cstdint>
#include <map>
#include <string>
#include "csv.h"

static ELFIO::elfio reader;
static ELFIO::section* dynsym = nullptr;

std::vector<Symbol> kioskSyms;

const std::vector<Symbol>& getSymbolsKiosk()
{
    return kioskSyms;
}
std::vector<Symbol> releaseSyms;

const std::vector<Symbol>& getSymbolsRelease()
{
    return releaseSyms;
}

void loadElfData()
{
    reader.load("Data/main.elf");
    
    for ( int i = 0; i < reader.sections.size(); ++i ) {
        ELFIO::section* psec = reader.sections[i];
        
        if ( psec->get_type() == ELFIO::SHT_DYNSYM )
            dynsym = psec;
    }

    const ELFIO::symbol_section_accessor symbols( reader, dynsym );
    for ( unsigned int i = 0; i < symbols.get_symbols_num(); i++ ) {
        std::string name;
        ELFIO::Elf64_Addr addr;
        ELFIO::Elf_Xword size;
        unsigned char bind;
        unsigned char type;
        ELFIO::Elf_Half section_index;
        unsigned char other;
        symbols.get_symbol( i, name, addr, size, bind, type, section_index, other );
        kioskSyms.push_back({ addr, size, name });
    }

    io::CSVReader<7, io::trim_chars<' ', '\t'>, io::double_quote_escape<',', '"'>> in("Data/main.csv");
    in.read_header(io::ignore_extra_column, "Name", "Location", "Type", "Namespace", "Source", "Reference Count", "Offcut Ref Count");

    std::string name, location, type, ns, source, refcount, offcutrefcount;
    while (in.read_row(name, location, type, ns, source, refcount, offcutrefcount))
    {
        ELFIO::Elf64_Addr addr;

        if (location == "External[ ? ]")
            addr = -1;
        else
            addr = std::stoi(location.substr(2).c_str(), nullptr, 16);
        std::string theName;

        if (ns != "Global")
        {
            theName = ns;
            theName.append("::");
        }

        theName.append(name);
        
        releaseSyms.push_back({ addr, 0, theName });
    }
}
