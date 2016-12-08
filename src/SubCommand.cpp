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

#include "SubCommand.hpp"

#include <cstdlib>

#include <iostream>
#include <string>
#include <vector>

std::vector<SubCommand *>
SubCommand::getAll()
{
    std::vector<SubCommand *> cmds;
    cmds.reserve(getAllCmds().size());
    for (const auto &cmd : getAllCmds()) {
        cmds.push_back(cmd.get());
    }
    return cmds;
}

std::vector<std::unique_ptr<SubCommand>> &
SubCommand::getAllCmds()
{
    static std::vector<std::unique_ptr<SubCommand>> allCmds;
    return allCmds;
}

int
SubCommand::exec(BuildHistory &bh, Repository &repo,
                 const std::vector<std::string> &args)
{
    hasErrors = false;

    if (args.size() < minArgs) {
        std::cout << "Too few arguments: " << args.size() << ".  "
                  << "Expected at least " << minArgs << '\n';
        error();
    }

    if (args.size() > maxArgs) {
        std::cout << "Too many arguments: " << args.size() << ".  "
                  << "Expected at most " << maxArgs << '\n';
        error();
    }

    if (!hasErrors) {
        bhValue = &bh;
        repoValue = &repo;

        execImpl(args);
    }

    return isFailed() ? EXIT_FAILURE : EXIT_SUCCESS;
}

void
SubCommand::error()
{
    hasErrors = true;
}
