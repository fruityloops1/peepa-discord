#include "SymbolData.h"
#include "csv.h"

void SymbolDB::load(const std::string& file)
{
    {
        io::CSVReader<4, io::trim_chars<' ', ','>, io::double_quote_escape<',', '"'>> in(file);
        in.read_header(io::ignore_extra_column, "Address", "Name", "Last Updated", "Last Updated User");

        symbols.clear();

        std::string addr, name, lastUpdated, lastUpdatedUser;
        while (in.read_row(addr, name, lastUpdated, lastUpdatedUser)) {
            uint32_t addrValue = std::stoi(addr, nullptr, 16);
            time_t lastUpdatedValue = std::stoul(lastUpdated);
            dpp::snowflake lastUpdatedUserValue = std::stoul(lastUpdatedUser);

            symbols.push_back({ addrValue, name, lastUpdatedValue, lastUpdatedUserValue });
        }
    }

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
