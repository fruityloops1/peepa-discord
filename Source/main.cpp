#include "appcommand.h"
#include "colors.h"
#include "elfdata.h"
#include "elfio/elf_types.hpp"
#include "message.h"
#include "util.h"
#include <dpp/cluster.h>
#include <dpp/dispatcher.h>
#include <sstream>
#include <stdio.h>
#include <dpp/dpp.h>
#include <string>
#include <iostream>
#include <variant>

constexpr const char* getVersionStr(int type)
{
    if (type == 0)
        return "Kiosk";
    else if (type == 1)
        return "1.1";
    return "???";
}

static std::string truncateString(const std::string& str) {
    if (str.length() > 2047) {
        std::string truncatedString = str.substr(0, 2044) + "...";
        return truncatedString;
    } else {
        return str;
    }
}

dpp::embed searchAddress(ELFIO::Elf64_Addr addr, const std::vector<Symbol>& symbols, int type)
{
    dpp::embed embed;

    std::stringstream foundSymbols;
    bool found = false;
    for (auto entry : symbols)
    {
        if (entry.addr == addr)
        {
            foundSymbols << entry.name << "\n";
            found = true;
        }
    }
    
    embed.set_title(format("Address Lookup (%s)", getVersionStr(type)));
    if (!found)
    {
        embed.set_color(dpp::colors::red);
        embed.set_description(format("No symbol was found at address 0x%.8x", addr));
    }
    else {
        embed.set_color(dpp::colors::green_apple);
        embed.set_description(format("The following symbols were found at 0x%.8x:", addr));
        embed.set_footer(truncateString(foundSymbols.str()), "");
    }

    return embed;
}

dpp::embed searchSymbols(std::string query, const std::vector<Symbol>& symbols, int type)
{
    dpp::embed embed;

    std::stringstream foundSymbols;
    bool found = false;
    for (auto entry : symbols)
    {
        if (entry.name.find(query) != std::string::npos)
        {
            foundSymbols << format("%s (0x%.8x)\n", entry.name.c_str(), entry.addr);
            found = true;
        }
    }
    
    embed.set_title(format("Symbol Search (%s)", getVersionStr(type)));
    if (!found)
    {
        embed.set_color(dpp::colors::red);
        embed.set_description(format("No symbols were found matching '%s'", query.c_str()));
    }
    else {
        embed.set_color(dpp::colors::green_apple);
        embed.set_description(format("The following symbols were found matching '%s':", query.c_str()));
        embed.set_footer(truncateString(foundSymbols.str()), "");
    }

    return embed;
}

int main()
{
    loadElfData();
    std::string token = readStringFromFile("Data/.token");
    dpp::cluster bot(token);
    bot.on_log(dpp::utility::cout_logger());

    bot.on_slashcommand([](const dpp::slashcommand_t& event) {
         if (event.command.get_command_name() == "symbolat") {
            ELFIO::Elf64_Addr addr(std::get<long>(event.get_parameter("address")));
            auto version = event.get_parameter("version");
            int versionId = 1;
            if (!version.valueless_by_exception() && std::holds_alternative<std::string>(version))
                versionId = std::get<std::string>(version) == "kiosk" ? 0 : 1;

            dpp::embed embed = searchAddress(addr, versionId == 0 ? getSymbolsKiosk() : getSymbolsRelease(), versionId);
            dpp::message reply;
            reply.add_embed(embed);
            event.reply(reply);
        } else if (event.command.get_command_name() == "searchsymbols")
        {
            std::string query(std::get<std::string>(event.get_parameter("query")));
            auto version = event.get_parameter("version");
            int versionId = 1;
            if (!version.valueless_by_exception() && std::holds_alternative<std::string>(version))
                versionId = std::get<std::string>(version) == "kiosk" ? 0 : 1;

            dpp::embed embed = searchSymbols(query, versionId == 0 ? getSymbolsKiosk() : getSymbolsRelease(), versionId);
            dpp::message reply;
            reply.add_embed(embed);
            event.reply(reply);
        }
    });

    bot.on_ready([&bot](const dpp::ready_t& event) {
        if (dpp::run_once<struct register_bot_commands>()) {
            dpp::slashcommand symbolAt(
                "symbolat",
                "Get Symbol(s) at the specified address",
                bot.me.id
            );

            symbolAt.add_option(dpp::command_option(dpp::co_integer, "address", "Address", true));
            symbolAt.add_option(
                dpp::command_option(dpp::co_string, "version", "Version", false)
                .add_choice(dpp::command_option_choice("1.1", std::string("1_1")))
                .add_choice(dpp::command_option_choice("Kiosk", std::string("kiosk")))
            );

            bot.global_command_create(symbolAt);

            dpp::slashcommand searchSymbols(
                "searchsymbols",
                "Searches symbols by query",
                bot.me.id
            );

            searchSymbols.add_option(dpp::command_option(dpp::co_string, "query", "Query", true));
            searchSymbols.add_option(
                dpp::command_option(dpp::co_string, "version", "Version", false)
                .add_choice(dpp::command_option_choice("1.1", std::string("1_1")))
                .add_choice(dpp::command_option_choice("Kiosk", std::string("kiosk")))
            );

            bot.global_command_create(searchSymbols);
        }
    });
    
    bot.start(dpp::st_wait);

}
