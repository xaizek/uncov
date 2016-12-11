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

#include "Invocation.hpp"

#include <stdexcept>
#include <string>
#include <vector>

Invocation::Invocation(const std::vector<std::string> &args)
{
    if (args.empty()) {
        throw std::invalid_argument("Broken argument list.");
    }

    if (args.size() < 3U) {
        error = "Usage: " + args[0] + " repo command [args...]";
        return;
    }

    repositoryPath = args[1];
    subcommandName = args[2];
    subcommandArgs.assign(args.cbegin() + 3, args.cend());
}

const std::string &
Invocation::getError() const
{
    return error;
}

const std::string &
Invocation::getRepositoryPath() const
{
    return repositoryPath;
}

const std::string &
Invocation::getSubcommandName() const
{
    return subcommandName;
}

const std::vector<std::string> &
Invocation::getSubcommandArgs() const
{
    return subcommandArgs;
}
