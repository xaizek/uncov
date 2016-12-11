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
#include <utility>
#include <vector>

Invocation::Invocation(std::vector<std::string> args)
{
    auto usageError = [this](const std::string &programName) {
        error = "Usage: " + programName + " [repo] command [args...]";
    };

    if (args.empty()) {
        throw std::invalid_argument("Broken argument list.");
    }

    // Extract program name.
    const std::string programName = args[0];
    args.erase(args.cbegin());

    if (args.empty()) {
        usageError(programName);
        return;
    }

    // Extract path to repository.
    auto isPath = [](const std::string &s) {
        return s.substr(0, 1) == "." || s.find('/') != std::string::npos;
    };
    if (isPath(args.front())) {
        repositoryPath = args.front();
        args.erase(args.cbegin());
    } else {
        repositoryPath = ".";
    }

    if (args.empty()) {
        usageError(programName);
        return;
    }

    // Extract subcommand and its arguments.
    subcommandName = args.front();
    args.erase(args.cbegin());
    subcommandArgs = std::move(args);
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
