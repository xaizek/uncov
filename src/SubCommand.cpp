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

#include <algorithm>
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
SubCommand::exec(BuildHistory &bh, Repository &repo, const std::string &alias,
                 const std::vector<std::string> &args)
{
    hasErrors = false;

    if (args.size() < minArgs) {
        std::cout << "Too few subcommand arguments: " << args.size() << ".  "
                  << makeExpectedMsg() << '\n';
        error();
    } else if (args.size() > maxArgs) {
        std::cout << "Too many subcommand arguments: " << args.size() << ".  "
                  << makeExpectedMsg() << '\n';
        error();
    }

    // Check that name matches one of aliases.
    if (std::find(names.cbegin(), names.cend(), alias) == names.cend()) {
        std::cout << "Unexpected subcommand name: " << alias << '\n';
        error();
    }

    if (!hasErrors) {
        bhValue = &bh;
        repoValue = &repo;

        execImpl(alias, args);
    }

    return hasErrors ? EXIT_FAILURE : EXIT_SUCCESS;
}

void
SubCommand::error()
{
    hasErrors = true;
}

std::string
SubCommand::makeExpectedMsg() const
{
    if (minArgs == maxArgs) {
        return "Expected exactly " + std::to_string(minArgs) + '.';
    }
    return "Expected at least " + std::to_string(minArgs) + " and at most "
         + std::to_string(maxArgs) + '.';
}
