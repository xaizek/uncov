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
#include <unordered_map>

#include "BuildHistory.hpp"
#include "DB.hpp"
#include "Repository.hpp"
#include "SubCommand.hpp"

int
main(int argc, char *argv[])
{
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " repo command [args...]\n";
        return EXIT_FAILURE;
    }

    std::unordered_map<std::string, SubCommand *> cmds;
    for (SubCommand *cmd : SubCommand::getAll()) {
        cmds.emplace(cmd->getName(), cmd);
    }

    auto cmd = cmds.find(argv[2]);
    if (cmd == cmds.end()) {
        std::cerr << "Unknown command: " << argv[2] << '\n';
        return EXIT_FAILURE;
    }

    Repository repo(argv[1]);
    DB db(repo.getGitPath() + "/uncover.sqlite");
    BuildHistory bh(db);

    return cmd->second->exec(bh, repo, { &argv[3], &argv[argc] });
}
