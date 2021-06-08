// Copyright (C) 2020 xaizek <xaizek@posteo.net>
//
// This file is part of uncov.
//
// uncov is free software: you can redistribute it and/or modify
// it under the terms of version 3 of the GNU Affero General Public License as
// published by the Free Software Foundation.
//
// uncov is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with uncov.  If not, see <http://www.gnu.org/licenses/>.

#include "Uncov.hpp"

#include <cstdlib>

#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "BuildHistory.hpp"
#include "DB.hpp"
#include "Invocation.hpp"
#include "Repository.hpp"
#include "Settings.hpp"
#include "SubCommand.hpp"
#include "TablePrinter.hpp"
#include "integration.hpp"

static void describeCommands(const std::map<std::string, SubCommand *> &cmds);

Uncov::Uncov(std::vector<std::string> args) : invocation(std::move(args))
{
    for (SubCommand *cmd : SubCommand::getAll()) {
        for (const std::string &name : cmd->getNames()) {
            cmds.emplace(name, cmd);
        }
    }
}

int
Uncov::run(Settings &settings)
{
    if (!invocation.getError().empty()) {
        std::cerr << "Usage error: " << invocation.getError() << "\n\n"
                  << invocation.getUsage() << '\n';
        return EXIT_FAILURE;
    }

    if (invocation.shouldPrintHelp()) {
        printHelp();
        return EXIT_SUCCESS;
    }

    if (invocation.shouldPrintVersion()) {
        std::cout << "uncov v0.4\n";
        return EXIT_SUCCESS;
    }

    auto cmd = cmds.find(invocation.getSubcommandName());
    if (cmd == cmds.end()) {
        std::cerr << "Unknown subcommand: "
                  << invocation.getSubcommandName() << '\n';
        return EXIT_FAILURE;
    }

    if (cmd->second->isGeneric()) {
        return cmd->second->exec(*this,
                                 invocation.getSubcommandName(),
                                 invocation.getSubcommandArgs());
    }

    Repository repo(invocation.getRepositoryPath());
    std::string gitPath = repo.getGitPath();

    settings.loadFromFile(gitPath + "/uncov.ini");

    DB db(gitPath + "/uncov.sqlite");
    BuildHistory bh(db);

    return cmd->second->exec(settings, bh, repo,
                             invocation.getSubcommandName(),
                             invocation.getSubcommandArgs());
}

void
Uncov::printHelp()
{
    std::cout << invocation.getUsage() << "\n\n";
    describeCommands(cmds);
}

void
Uncov::printHelp(const std::string &alias)
{
    auto cmd = cmds.find(alias);
    if (cmd == cmds.end()) {
        throw std::invalid_argument("Unknown subcommand: " + alias);
    }

    std::cout << alias << "\n\n"
              << cmd->second->getDescription(alias) << "\n\n";
    cmd->second->printHelp(std::cout, alias);
}

/**
 * @brief Prints information about all commands on standard output.
 *
 * @param cmds All available commands and aliases.
 */
static void
describeCommands(const std::map<std::string, SubCommand *> &cmds)
{
    std::cout << "Subcommands\n";

    TablePrinter tablePrinter {
        { "-Name", "-Description" }, getTerminalSize().first, true
    };

    for (const auto &entry : cmds) {
        tablePrinter.append({ "   " + entry.first,
                              entry.second->getDescription(entry.first) });
    }

    RedirectToPager redirectToPager;
    tablePrinter.print(std::cout);
}
