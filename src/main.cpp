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
#include <stdexcept>
#include <unordered_map>

#include "BuildHistory.hpp"
#include "DB.hpp"
#include "Invocation.hpp"
#include "Repository.hpp"
#include "SubCommand.hpp"

int
main(int argc, char *argv[])
{
    try {
        Invocation invocation({ &argv[0], &argv[argc] });

        if (!invocation.getError().empty()) {
            std::cerr << "Usage error: " << invocation.getError() << '\n';
            return EXIT_FAILURE;
        }

        std::unordered_map<std::string, SubCommand *> cmds;
        for (SubCommand *cmd : SubCommand::getAll()) {
            for (const std::string &name : cmd->getNames()) {
                cmds.emplace(name, cmd);
            }
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

        return cmd->second->exec(bh, repo, invocation.getSubcommandName(),
                                 invocation.getSubcommandArgs());
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
}
