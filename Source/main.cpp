#include "SymbolData.h"
#include "appcommand.h"
#include "colors.h"
#include "elfdata.h"
#include "elfio/elf_types.hpp"
#include "guild.h"
#include "mergeconflict.h"
#include "message.h"
#include "restresults.h"
#include "unicode_emoji.h"
#include "util.h"
#include <ctime>
#include <dpp/cluster.h>
#include <dpp/dispatcher.h>
#include <dpp/dpp.h>
#include <iostream>
#include <mutex>
#include <sstream>
#include <stdio.h>
#include <string>
#include <variant>
#include "httplib.h"

SymbolDB sDatabase;
constexpr const char* sDatabaseFile = "db.csv";
std::mutex sDatabaseLock;
dpp::cluster* bot = nullptr;

dpp::embed lookupCommand(uint64_t address)
{
    dpp::embed embed;
    embed.set_title(format("Symbol Lookup for 71%.8X", address));
    embed.set_color(dpp::colors::red_blood);
    embed.set_description(format("No symbol found at address 71%.8X", address));

    bool found = sDatabase.symbols.contains(address);
    if (found)
    {
        Symbol symbol = sDatabase.symbols[address];
        embed.set_color(dpp::colors::green_apple);
        std::string display = symbol.name;
        std::string demangled = demangle(symbol.name);
        if (symbol.name.find("_Z") != std::string::npos)
            display = format("%s\n%s", symbol.name.c_str(), demangled.c_str());
        
        embed.set_footer(truncateString(display), "");
        embed.set_description(format("Last modified on <t:%zu:f> by <@%zu>", symbol.lastUpdated, symbol.updatedUser));
    }

    std::string commentDisplay;
    for (auto comment : sDatabase.comments)
    {
        if (comment.addr == address)
        {
            if (!found)
            {
                embed.set_color(dpp::colors::blue_diamond);
                embed.set_description(format("", address));
            }
            commentDisplay.append(format("\n\n%s:\n%s", dpp::user::get_mention(comment.user).c_str(), comment.comment.c_str()));
        }
    }
    if (!commentDisplay.empty())
    {
        embed.set_description(embed.description + "\n" + truncateString(format("%s:%s", found ? "Comments" : format("No symbol found at 71%.8X, but comments were found", address).c_str(), commentDisplay.c_str()), 4096));
    }
    
    return embed;
}

dpp::embed fail(const std::string& msg, const std::string description = "")
{
    dpp::embed embed;
    embed.set_title(truncateString("❌ " + msg, 256));
    embed.set_color(dpp::colors::red_blood);
    embed.set_description(truncateString(description, 4096));
    return embed;
}

dpp::embed success(const std::string& msg, const std::string& footer = "", const std::string& description = "")
{
    dpp::embed embed;
    embed.set_title("✅ " + msg);
    embed.set_color(dpp::colors::green_apple);
    if (!footer.empty())
        embed.set_footer(footer, "");
    if (!description.empty())
        embed.set_description(description);
    return embed;
}

constexpr dpp::snowflake sPermissionRole = 1163869548961874021;
constexpr dpp::snowflake sNotifChannel = 1164550169350639656;
constexpr dpp::snowflake sPermitted[] {
    339842109748019201
};

bool haspermission(const dpp::guild_member& user)
{
    for (auto role : user.get_roles())
        if (role == sPermissionRole)
            return true;
    for (auto permittedId : sPermitted)
        if (permittedId == user.user_id)
            return true;
    return false;
}

std::vector<std::string> splitString(const std::string& input, char delimiter) {
    std::vector<std::string> result;
    std::string substring;
    
    for (char c : input) {
        if (c == delimiter) {
            if (!substring.empty()) {
                result.push_back(substring);
                substring.clear();
            }
        } else {
            substring.push_back(c);
        }
    }
    
    if (!substring.empty()) {
        result.push_back(substring);
    }
    
    return result;
}

std::vector<std::pair<Symbol, Symbol>> findConflicts(SymbolDB& with)
{
    std::vector<std::pair<Symbol, Symbol>> conflicts;

    for (auto symbol : with.symbols)
    {
        if (sDatabase.symbols.contains(symbol.first))
        {
            Symbol origSymbol = sDatabase.symbols[symbol.first];
            if (origSymbol.name != symbol.second.name)
            {
                conflicts.push_back({ origSymbol, symbol.second });
            }
        }
    }
    return conflicts;
}

std::map<std::string, std::pair<SymbolDB, dpp::snowflake>> sConflicts;
std::map<dpp::snowflake, std::string> sApiTokens;

static std::string mergeDBsInternal(SymbolDB& with, dpp::snowflake author)
{
    auto conflicts = findConflicts(with);
    
    if (conflicts.empty())
    {
        std::lock_guard<std::mutex> lock(sDatabaseLock);
        for (auto symbol : with.symbols)
        {
            Symbol& newSym = sDatabase.symbols[symbol.first];
            newSym = symbol.second;
            newSym.lastUpdated = time(nullptr);
            newSym.updatedUser = author;
        }

        return "success";
    }
    else {
        std::string id = generateRandomString(24);
        sConflicts[id] = { with, author };
        return format("http://wsdb.littun.co/conflict/%s/", id.c_str());
    }
}

static void mergeDBs(SymbolDB& with, const dpp::message_create_t& msgEvent)
{ 
    auto conflicts = findConflicts(with);

    if (conflicts.empty())
    {
        std::lock_guard<std::mutex> lock(sDatabaseLock);
        for (auto symbol : with.symbols)
        {
            Symbol& newSym = sDatabase.symbols[symbol.first];
            newSym = symbol.second;
            newSym.lastUpdated = time(nullptr);
            newSym.updatedUser = msgEvent.msg.author.id;
        }

        dpp::embed embed = success("Successfully merged into symbol database");
        dpp::message msg;
        msg.add_embed(embed);
        msgEvent.reply(msg);
        sDatabase.save(sDatabaseFile);
    }
    else {
        std::string id = generateRandomString(24);
        sConflicts[id] = { with, msgEvent.msg.author.id };
        dpp::embed embed = fail("Conflicts found", format("Solve conflicts at http://wsdb.littun.co/conflict/%s/", id.c_str()));
        dpp::message msg;
        msg.set_flags(dpp::m_ephemeral);
        msg.add_embed(embed);
        msgEvent.reply(msg);
    }
}

static void httpThread()
{
    httplib::Server server;

    server.Get("/api/symbols", [](const httplib::Request& req, httplib::Response &res) {
        std::lock_guard<std::mutex> lock(sDatabaseLock);
        res.set_content(sDatabase.saveSymbols(), "text/plain");
    });

    server.Get("/conflict/:conflictid", [](const httplib::Request& req, httplib::Response &res) {
        auto conflict_id = req.path_params.at("conflictid");
        if (sConflicts.contains(conflict_id))
        {
            auto conflictdb = sConflicts[conflict_id];
            auto conflicts = findConflicts(conflictdb.first);
            std::string outPage = sConflictFmt1;
            int i = 0;
            for (auto conflict : conflicts)
            {
                auto demangleIfNecessary = [](const std::string& symbol) {
                    if (symbol.find("_Z") != std::string::npos)
                        return format("%s (%s)", demangle(symbol).c_str(), symbol.c_str());
                    return symbol;  
                };
                std::string oldSym = demangleIfNecessary(conflict.first.name);
                std::string newSym = demangleIfNecessary(conflict.second.name);
                outPage.append(format(sConflictEntryFmt, conflict.first.addr, i, i, i, oldSym.c_str(), i, i, i, newSym.c_str()));
                i++;
            }
            outPage.append(sConflictFmt2);
            res.set_content(outPage, "text/html");
        }
        else
            res.set_content("Invalid conflict ID", "text/plain");
    });

    server.Post("/conflict/:conflictid/submit_resolution", [](const httplib::Request& req, httplib::Response &res) {
        auto conflict_id = req.path_params.at("conflictid");
        if (sConflicts.contains(conflict_id))
        {
            if (req.body.find("SUPERDUPERCANCELCANCELTHISSHITSOWECANNOTMERGETHESYMBOLSATALL") != std::string::npos)
            {
                auto conflictdb = sConflicts[conflict_id];
                sConflicts.erase(conflict_id);
                dpp::embed embed = fail("Note", format("<@%zu> has cancelled conflict", conflictdb.second));
                dpp::message msg;
                msg.add_embed(embed);
                msg.channel_id = sNotifChannel;
                bot->message_create(msg);
                return;
            }
            std::vector<std::string> options = splitString(req.body, '&');
            std::vector<bool> decisions;
            for (std::string option : options)
            {
                std::vector<std::string> sides = splitString(option, '=');
                decisions.push_back(sides[1] == "new");
            }

            {
                std::lock_guard<std::mutex> lock(sDatabaseLock);
                auto conflictdb = sConflicts[conflict_id];
                auto conflicts = findConflicts(conflictdb.first);
                std::cout << conflicts.size() << std::endl;

                for (int i = 0; i < conflicts.size(); i++)
                {
                    Symbol& symbol = sDatabase.symbols[conflicts[i].first.addr];
                    symbol = decisions[i] ? conflicts[i].second : conflicts[i].first;
                    std::cout << symbol.name << std::endl;
                    symbol.lastUpdated = time(nullptr);
                    symbol.updatedUser = conflictdb.second;
                }

                auto isInConflicts = [&conflicts](uint64_t address) {
                    for (const auto conflict : conflicts)
                    {
                        if (conflict.first.addr == address)
                            return true;
                    }
                    return false;
                };

                for (auto entry : conflictdb.first.symbols)
                {
                    if (!isInConflicts(entry.first) && !sDatabase.symbols.contains(entry.first))
                    {
                        sDatabase.symbols[entry.first] = entry.second;
                        sDatabase.symbols[entry.first].lastUpdated = time(nullptr);
                        sDatabase.symbols[entry.first].updatedUser = conflictdb.second;
                    }
                }

                dpp::embed embed = success("Note", "", format("<@%zu> has solved conflicts", conflictdb.second));
                dpp::message msg;
                msg.add_embed(embed);
                msg.channel_id = sNotifChannel;
                bot->message_create(msg);

                sDatabase.save(sDatabaseFile);
                res.status = 200;
            }
            sConflicts.erase(conflict_id);
        }
    });

    server.Post("/api/upload", [](const httplib::Request& req, httplib::Response& res) {
        if (!sConflicts.empty())
        {
            res.status = 503;
            return;
        }
        if (req.has_header("Authorization"))
        {
            std::string authorization = req.get_header_value("Authorization");
            dpp::snowflake user;
            bool found = false;
            for (auto entry : sApiTokens)
            {
                if (entry.second == authorization)
                {
                    user = entry.first;
                    found = true;
                }
            }

            if (found) {
                res.status = 200;
                
                SymbolDB givenDB;
                try {
                    std::istringstream stream(req.body);
                    givenDB.loadFromStream(stream, "");
                } catch (std::exception& e)
                {
                    res.status = 400;
                    res.set_header("error", e.what());
                    return;
                }

                std::string out = mergeDBsInternal(givenDB, user);
                res.status = out == "success" ? 200 : 201;
                res.set_content(out, "text/plain");

                dpp::embed embed = success("Symbol database has been updated", "", format("<@%zu> has updated the [symbol database](https://wsdb.littun.co/api/symbols) using the API", user));
                dpp::message msg;
                msg.add_embed(embed);
                msg.channel_id = sNotifChannel;
                bot->message_create(msg);
            }
            else
                res.status = 401;
        } else {
            res.status = 401;
        }
    });

    server.listen("0.0.0.0", 5224);
}

int main()
{
    sDatabase.load(sDatabaseFile);
    // loadElfData();
    std::string token = readStringFromFile("Data/.token");
    bot = new dpp::cluster(token, dpp::intents::i_default_intents | dpp::intents::i_message_content);
    bot->on_log(dpp::utility::cout_logger());
    std::thread http(httpThread);

    bot->on_slashcommand([](const dpp::slashcommand_t& event) {
        if (event.command.get_command_name() == "lookup") {
            uint64_t addr(std::get<long>(event.get_parameter("address")));
            if (addr >= 0x7100000000)
                addr -= 0x7100000000;
            dpp::embed embed = lookupCommand(addr);
            dpp::message msg;
            msg.add_embed(embed);
            event.reply(msg);
        } else if (event.command.get_command_name() == "demangle") {
            std::string symbol(std::get<std::string>(event.get_parameter("symbol")));
            std::string demangled = demangle(symbol);
            event.reply(format("`%s`", demangled.c_str()));
        } else if (event.command.get_command_name() == "setsymbol") {
            uint64_t addr(std::get<long>(event.get_parameter("address")));
            std::string symbol(std::get<std::string>(event.get_parameter("symbol")));
            if (addr >= 0x7100000000)
                addr -= 0x7100000000;

            size_t pos = symbol.find("\"");
            while (pos != std::string::npos)
            {
                symbol.replace(pos, 1, "\"\"");
                pos = symbol.find("\"");
            }
            pos = symbol.find("\n");
            while (pos != std::string::npos)
            {
                symbol.replace(pos, 1, "");
                pos = symbol.find("\n");
            }

            dpp::message msg;
            if (!haspermission(event.command.member))
            {
                msg.add_embed(fail("You don't have permission to edit the symbol database"));
                msg.set_flags(dpp::m_ephemeral);
                event.reply(msg);
                return;
            }

            if (isGarbageSymbol(symbol))
            {
                msg.add_embed(fail("You need to provide a real symbol"));
                msg.set_flags(dpp::m_ephemeral);
                event.reply(msg);
                return;
            }

            {
                std::lock_guard<std::mutex> lock(sDatabaseLock);
                sDatabase.symbols[addr] = {uint32_t(addr), symbol, time(nullptr), event.command.member.user_id};
                sDatabase.save(sDatabaseFile);
            }
            
            msg.add_embed(success("Successfully set symbol"));
            msg.set_flags(dpp::m_ephemeral);
            event.reply(msg);
        } else if (event.command.get_command_name() == "addcomment") {
            uint64_t addr(std::get<long>(event.get_parameter("address")));
            std::string comment(std::get<std::string>(event.get_parameter("comment")));
            if (addr >= 0x7100000000)
                addr -= 0x7100000000;

            dpp::message msg;
            if (!haspermission(event.command.member))
            {
                msg.add_embed(fail("You don't have permission to edit the symbol database"));
                msg.set_flags(dpp::m_ephemeral);
                event.reply(msg);
                return;
            }

            {
                std::lock_guard<std::mutex> lock(sDatabaseLock);
                sDatabase.comments.push_back({uint32_t(addr), comment, time(nullptr), event.command.member.user_id});
                sDatabase.save(sDatabaseFile);
            }

            msg.add_embed(success("Successfully added comment"));
            msg.set_flags(dpp::m_ephemeral);
            event.reply(msg);
        } else if (event.command.get_command_name() == "apitoken") {
            dpp::message msg;
            if (!haspermission(event.command.member))
            {
                msg.add_embed(fail("You don't have permission to edit the symbol database"));
                msg.set_flags(dpp::m_ephemeral);
                event.reply(msg);
                return;
            }

            sApiTokens[event.command.member.user_id] = generateRandomString(56);

            msg.add_embed(success("Successfully generated API token", format("Your token is %s\nPlease note that this token is regenerated with each use of this command.", sApiTokens[event.command.member.user_id].c_str())));
            msg.set_flags(dpp::m_ephemeral);
            event.reply(msg);
        } else if (event.command.get_command_name() == "removesymbol") 
        {
            uint64_t addr(std::get<long>(event.get_parameter("address")));
            if (addr >= 0x7100000000)
                addr -= 0x7100000000;

            dpp::message msg;
            if (!haspermission(event.command.member))
            {
                msg.add_embed(fail("You don't have permission to edit the symbol database"));
                msg.set_flags(dpp::m_ephemeral);
                event.reply(msg);
                return;
            }

            {
                std::lock_guard<std::mutex> lock(sDatabaseLock);
                if (sDatabase.symbols.contains(addr))
                {
                    sDatabase.symbols.erase(addr);
                    sDatabase.save(sDatabaseFile);


                    msg.add_embed(fail(format("Successfully removed symbol at 71%.8X", addr)));
                    msg.set_flags(dpp::m_ephemeral);
                    event.reply(msg);
                } else {
                    msg.add_embed(fail(format("No symbol exists at 71%.8X", addr)));
                    msg.set_flags(dpp::m_ephemeral);
                    event.reply(msg);
                }
            }
        }
        else if (event.command.get_command_name() == "search")
        {
            std::string query(std::get<std::string>(event.get_parameter("query")));

            {
                std::lock_guard<std::mutex> lock(sDatabaseLock);

                dpp::embed embed;
                embed.set_title("Symbol Search");
                embed.set_color(dpp::colors::bashful_pink);

                std::string print;

                auto demangleIfNecessary = [](const std::string& symbol) {
                    if (symbol.find("_Z") != std::string::npos)
                        return format("%s (%s)", demangle(symbol).c_str(), symbol.c_str());
                    return symbol;  
                };

                for (auto symbol : sDatabase.symbols)
                {
                    if (symbol.second.name.find(query) != std::string::npos)
                        print += format("%s - 71%.8X\n", demangleIfNecessary(symbol.second.name).c_str(), symbol.first);
                }

                embed.set_description(print.empty() ? "No symbols found with query" : print);
                dpp::message msg;
                msg.add_embed(embed);
                event.reply(msg);
            }
        }
    });

    bot->on_message_create([](const dpp::message_create_t& msgEvent) {
        bool mentionsMe = false;
        for (auto mention : msgEvent.msg.mentions)
        {
            if (mention.first.id == bot->me.id)
                mentionsMe = true;
        }
        if (mentionsMe && !msgEvent.msg.attachments.empty())
        {
            dpp::message msg;
            if (!haspermission(msgEvent.msg.member))
            {
                msg.add_embed(fail("You don't have permission to edit the symbol database"));
                msg.set_flags(dpp::m_ephemeral);
                msgEvent.reply(msg);
                return;
            }
            if (!sConflicts.empty())
            {
                dpp::embed embed = fail("Database is busy", "Someone else is currently solving merge conflicts. Please wait for them to finish or cancel");
                dpp::message msg;
                msg.add_embed(embed);
                msgEvent.reply(msg);
                return;
            }
            msgEvent.msg.attachments[0].download([msgEvent](const dpp::http_request_completion_t& event) {
                if (event.status != 200)
                {
                    dpp::embed embed = fail("Could not download file", format("HTTP request returned code %d", event.status));
                    dpp::message msg;
                    msg.add_embed(embed);
                    msgEvent.reply(msg);
                    return;
                }

                if (event.body.find("\"Address\",\"Name\",\"Last Updated\",\"Last Updated User\"") != std::string::npos)
                {
                    SymbolDB givenDB;
                    try {
                        std::istringstream stream(event.body);
                        givenDB.loadFromStream(stream, "");
                    } catch (std::exception& e)
                    {
                        dpp::embed embed = fail("Exception ocurred while parsing symbol map", e.what());
                        dpp::message msg;
                        msg.add_embed(embed);
                        msgEvent.reply(msg);
                        return;
                    }

                    mergeDBs(givenDB, msgEvent);
                }
            });
        }
    });

    bot->on_ready([](const dpp::ready_t& event) {
        if (dpp::run_once<struct register_bot_commands>()) {
            // dpp::slashcommand symbolAt(
            //     "symbolat",
            //     "Get Symbol(s) at the specified address",
            //     bot->me.id);

            // symbolAt.add_option(dpp::command_option(dpp::co_integer, "address", "Address", true));
            // symbolAt.add_option(
            //     dpp::command_option(dpp::co_string, "version", "Version", false)
            //         .add_choice(dpp::command_option_choice("1.1", std::string("1_1")))
            //         .add_choice(dpp::command_option_choice("Kiosk", std::string("kiosk")))
            //         .add_choice(dpp::command_option_choice("Wii U (EU)", std::string("wiiu"))));

            // bot->global_command_create(symbolAt);

            dpp::slashcommand demangleCmd(
                "demangle",
                "Demangles C++ Symbol",
                bot->me.id);

            demangleCmd.add_option(dpp::command_option(dpp::co_string, "symbol", "Mangled symbol", true));

            dpp::slashcommand lookupCmd(
                "lookup",
                "Lookup symbol for address",
                bot->me.id);
            
            lookupCmd.add_option(dpp::command_option(dpp::co_integer, "address", "The address to be looked up", true));


            dpp::slashcommand searchCmd(
                "search",
                "Search for symbol by query",
                bot->me.id);
            
            searchCmd.add_option(dpp::command_option(dpp::co_string, "query", "The search query", true));

            dpp::slashcommand setCmd(
                "setsymbol",
                "Set symbol for address",
                bot->me.id);
            
            setCmd.add_option(dpp::command_option(dpp::co_integer, "address", "The address of the symbol", true));
            setCmd.add_option(dpp::command_option(dpp::co_string, "symbol", "The symbol to be set", true));

            dpp::slashcommand removeSymCmd(
                "removesymbol",
                "Remove symbol from address",
                bot->me.id);
            removeSymCmd.add_option(dpp::command_option(dpp::co_integer, "address", "The address of the symbol", true));

            dpp::slashcommand addCommentCmd(
                "addcomment",
                "Add comment to address",
                bot->me.id);

            addCommentCmd.add_option(dpp::command_option(dpp::co_integer, "address", "Address", true));
            addCommentCmd.add_option(dpp::command_option(dpp::co_string, "comment", "The comment to be added", true));

            
            dpp::slashcommand apiTokenCmd(
                "apitoken",
                "Generate new API token",
                bot->me.id);

            bot->global_command_create(demangleCmd);
            bot->global_command_create(searchCmd);
            bot->global_command_create(lookupCmd);
            bot->global_command_create(setCmd);
            bot->global_command_create(addCommentCmd);
            bot->global_command_create(apiTokenCmd);
            bot->global_command_create(removeSymCmd);
        }
    });

    bot->start(dpp::st_wait);
}
