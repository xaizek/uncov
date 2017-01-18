// Copyright (C) 2016 xaizek <xaizek@openmailbox.org>
//
// This file is part of uncov.
//
// uncov is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// uncov is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with uncov.  If not, see <http://www.gnu.org/licenses/>.

#include <cstdlib>

#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>

#include "BuildHistory.hpp"
#include "DB.hpp"
#include "Invocation.hpp"
#include "Repository.hpp"
#include "Settings.hpp"
#include "SubCommand.hpp"
#include "TablePrinter.hpp"
#include "integration.hpp"

static void describeCommands(const std::map<std::string, SubCommand *> &cmds);

int
main(int argc, char *argv[])
{
    auto settings = std::make_shared<Settings>();
    PrintingSettings::set(settings);

    try {
        Invocation invocation({ &argv[0], &argv[argc] });

        if (!invocation.getError().empty()) {
            std::cerr << "Usage error: " << invocation.getError() << "\n\n"
                      << invocation.getUsage() << '\n';
            return EXIT_FAILURE;
        }

        std::map<std::string, SubCommand *> cmds;
        for (SubCommand *cmd : SubCommand::getAll()) {
            for (const std::string &name : cmd->getNames()) {
                cmds.emplace(name, cmd);
            }
        }

        if (invocation.shouldPrintHelp()) {
            std::cout << invocation.getUsage() << "\n\n";
            describeCommands(cmds);
            return EXIT_SUCCESS;
        }

        if (invocation.shouldPrintVersion()) {
            std::cout << "uncov v0.1\n";
            return EXIT_SUCCESS;
        }

        auto cmd = cmds.find(invocation.getSubcommandName());
        if (cmd == cmds.end()) {
            std::cerr << "Unknown subcommand: "
                      << invocation.getSubcommandName() << '\n';
            return EXIT_FAILURE;
        }

        Repository repo(invocation.getRepositoryPath());
        DB db(repo.getGitPath() + "/uncov.sqlite");
        BuildHistory bh(db);

        return cmd->second->exec(*settings, bh, repo,
                                 invocation.getSubcommandName(),
                                 invocation.getSubcommandArgs());
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
}

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
