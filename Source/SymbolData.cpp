#include "SymbolData.h"
#include "csv.h"
#include <fstream>
#include <iomanip>
#include <mutex>

void SymbolDB::load(const std::string& file)
{
    std::ifstream stream(file);
    loadFromStream(stream, file);

    std::string commentsFile = file;
    commentsFile.append(".comments");
    io::CSVReader<4, io::trim_chars<' ', ','>, io::double_quote_escape<',', '"'>> in(commentsFile);
    in.read_header(io::ignore_extra_column, "Address", "Comment", "Time Added", "User");

    std::string addr, comment, added, userId;
    while (in.read_row(addr, comment, added, userId)) {
        uint32_t addrValue = std::stoi(addr, nullptr, 16);
        time_t addedValue = std::stoul(added);
        dpp::snowflake userIdValue = std::stoul(userId);

        comments.push_back({ addrValue, comment, addedValue, userIdValue });
    }
}

void SymbolDB::loadFromStream(std::istream& data, const std::string& filename)
{
    io::CSVReader<4, io::trim_chars<' ', ','>, io::double_quote_escape<',', '"'>> in(filename, data);
    in.read_header(io::ignore_extra_column, "Address", "Name", "Last Updated", "Last Updated User");

    symbols.clear();

    std::string addr, name, lastUpdated, lastUpdatedUser;
    while (in.read_row(addr, name, lastUpdated, lastUpdatedUser)) {
        uint32_t addrValue = std::stoi(addr, nullptr, 16);
        time_t lastUpdatedValue = std::stoul(lastUpdated);
        dpp::snowflake lastUpdatedUserValue = std::stoul(lastUpdatedUser);

        symbols[addrValue] = { addrValue, name, lastUpdatedValue, lastUpdatedUserValue };
    }
    
}

std::string SymbolDB::saveSymbols()
{
    std::stringstream out;
    out << "\"Address\",\"Name\",\"Last Updated\",\"Last Updated User\"\n";

    for (auto symbol : symbols)
    {
        out << "\"" << std::hex << std::setw(8) << std::setfill('0') << symbol.second.addr << "\",";
        out << "\"" << std::dec << symbol.second.name << "\",";
        out << "\"" << std::dec << symbol.second.lastUpdated << "\",";
        out << "\"" << std::dec << symbol.second.updatedUser << "\"\n";
    }
    return out.str();
}

void SymbolDB::save(const std::string& file)
{
    {
        std::ofstream out(file);

        out << saveSymbols();
    }

    std::string fileComments(file);
    fileComments.append(".comments");
    std::ofstream out(fileComments);

    out << "\"Address\",\"Comment\",\"Time Added\",\"User\"\n";

    for (auto comment : comments)
    {
        out << "\"" << std::hex << std::setw(8) << std::setfill('0') << comment.addr << "\",";
        out << "\"" << std::dec << comment.comment << "\",";
        out << "\"" << std::dec << comment.added << "\",";
        out << "\"" << std::dec << comment.user << "\"\n";
    }
}
