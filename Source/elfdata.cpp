#include "elfdata.h"
#include "csv.h"
#include "elfio/elf_types.hpp"
#include "elfio/elfio.hpp"
#include <cstdint>
#include <map>
#include <string>

static std::string executeCommand(const std::string& command)
{
    std::array<char, 128> buffer;
    std::string result;
    std::shared_ptr<FILE> pipe(popen(command.c_str(), "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

std::string demangle(const std::string& input)
{
    std::string command = "bash Data/demangle \"" + input + "\"";
    return executeCommand(command);
}

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

std::vector<Symbol> wiiuEuSyms;

const std::vector<Symbol>& getSymbolsWiiuEu()
{
    return wiiuEuSyms;
}

void loadElfData()
{
    reader.load("Data/main.elf");

    for (int i = 0; i < reader.sections.size(); ++i) {
        ELFIO::section* psec = reader.sections[i];

        if (psec->get_type() == ELFIO::SHT_DYNSYM)
            dynsym = psec;
    }

    const ELFIO::symbol_section_accessor symbols(reader, dynsym);
    for (unsigned int i = 0; i < symbols.get_symbols_num(); i++) {
        std::string name;
        ELFIO::Elf64_Addr addr;
        ELFIO::Elf_Xword size;
        unsigned char bind;
        unsigned char type;
        ELFIO::Elf_Half section_index;
        unsigned char other;
        symbols.get_symbol(i, name, addr, size, bind, type, section_index, other);
        kioskSyms.push_back({ addr, size, name });
    }

    struct Set {
        std::string file;
        std::vector<Symbol>& ref;
    };

    Set sets[2] = { { "Data/main.csv", releaseSyms },
        { "Data/wiiu.csv", wiiuEuSyms } };

    for (Set set : sets) {
        io::CSVReader<7, io::trim_chars<' ', '\t'>, io::double_quote_escape<',', '"'>> in(set.file);
        in.read_header(io::ignore_extra_column, "Name", "Location", "Type", "Namespace", "Source", "Reference Count", "Offcut Ref Count");

        std::string name, location, type, ns, source, refcount, offcutrefcount;
        while (in.read_row(name, location, type, ns, source, refcount, offcutrefcount)) {
            ELFIO::Elf64_Addr addr;

            if (location == "External[ ? ]")
                addr = -1;
            else
                addr = std::stoi(location.substr(2).c_str(), nullptr, 16);
            std::string theName;

            if (ns != "Global") {
                theName = ns;
                theName.append("::");
            }

            theName.append(name);

            set.ref.push_back({ set.file == "Data/wiiu.csv" ? addr + 0x02000000 : addr, 0, theName });
        }
    }
}
