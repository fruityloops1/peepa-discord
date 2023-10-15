#include "SymbolData.h"
#include "appcommand.h"
#include "colors.h"
#include "elfdata.h"
#include "elfio/elf_types.hpp"
#include "message.h"
#include "util.h"
#include <dpp/cluster.h>
#include <dpp/dispatcher.h>
#include <dpp/dpp.h>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string>
#include <variant>

constexpr const char* getVersionStr(int type)
{
    if (type == 0)
        return "Kiosk";
    else if (type == 1)
        return "1.1";
    else if (type == 2)
        return "Wii U (EU)";
    return "???";
}

int main()
{
    SymbolDB db;
    db.load("a.csv");
    return 0;
    // loadElfData();
    std::string token = readStringFromFile("Data/.token");
    dpp::cluster bot(token);
    bot.on_log(dpp::utility::cout_logger());

    bot.on_slashcommand([](const dpp::slashcommand_t& event) {
        if (event.command.get_command_name() == "symbolat") {
            ELFIO::Elf64_Addr addr(std::get<long>(event.get_parameter("address")));
        } else if (event.command.get_command_name() == "searchsymbols") {
        } else if (event.command.get_command_name() == "demangle") {
            std::string symbol(std::get<std::string>(event.get_parameter("symbol")));
            std::string demangled = demangle(symbol);
            event.reply(format("`%s`", demangled.c_str()));
        }
    });

    bot.on_ready([&bot](const dpp::ready_t& event) {
        if (dpp::run_once<struct register_bot_commands>()) {
            // dpp::slashcommand symbolAt(
            //     "symbolat",
            //     "Get Symbol(s) at the specified address",
            //     bot.me.id);

            // symbolAt.add_option(dpp::command_option(dpp::co_integer, "address", "Address", true));
            // symbolAt.add_option(
            //     dpp::command_option(dpp::co_string, "version", "Version", false)
            //         .add_choice(dpp::command_option_choice("1.1", std::string("1_1")))
            //         .add_choice(dpp::command_option_choice("Kiosk", std::string("kiosk")))
            //         .add_choice(dpp::command_option_choice("Wii U (EU)", std::string("wiiu"))));

            // bot.global_command_create(symbolAt);

            dpp::slashcommand demangleCmd(
                "demangle",
                "Demangles C++ Symbol",
                bot.me.id);

            demangleCmd.add_option(dpp::command_option(dpp::co_string, "symbol", "Mangled symbol", true));

            bot.global_command_create(demangleCmd);
        }
    });

    bot.start(dpp::st_wait);
}
