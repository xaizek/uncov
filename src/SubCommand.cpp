// Copyright (C) 2016 xaizek <xaizek@posteo.net>
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

#include "SubCommand.hpp"

#include <cassert>
#include <cstdlib>

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

SubCommand::~SubCommand()
{
    for (const std::string &alias : names) {
        if (descriptions[alias].empty()) {
            assert(false && "An alias lacks description.");
        }
    }
}

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
SubCommand::exec(Uncov &uncov, const std::string &alias,
                 const std::vector<std::string> &args)
{
    if (!isGeneric()) {
        throw std::logic_error("Repo-command is invoked using app-command "
                               "interface");
    }

    hasErrors = false;
    checkExec(alias, args);

    if (!hasErrors) {
        uncovValue = &uncov;

        execImpl(alias, args);
    }

    return hasErrors ? EXIT_FAILURE : EXIT_SUCCESS;
}

int
SubCommand::exec(Settings &settings, BuildHistory &bh, Repository &repo,
                 const std::string &alias, const std::vector<std::string> &args)
{
    if (isGeneric()) {
        throw std::logic_error("App-command is invoked using repo-command "
                               "interface");
    }

    hasErrors = false;
    checkExec(alias, args);

    if (!hasErrors) {
        settingsValue = &settings;
        bhValue = &bh;
        repoValue = &repo;

        execImpl(alias, args);
    }

    return hasErrors ? EXIT_FAILURE : EXIT_SUCCESS;
}

void
SubCommand::checkExec(const std::string &alias,
                      const std::vector<std::string> &args)
{
    if (!isAlias(alias)) {
        std::cout << "Unexpected subcommand name: " << alias << '\n';
        error();
    }

    if (args.size() < minArgs) {
        std::cout << "Too few subcommand arguments: " << args.size() << ".  "
                  << makeExpectedMsg() << '\n';
        error();
    } else if (args.size() > maxArgs) {
        std::cout << "Too many subcommand arguments: " << args.size() << ".  "
                  << makeExpectedMsg() << '\n';
        error();
    }
}

void
SubCommand::describe(const std::string &alias, const std::string &descr)
{
    if (!isAlias(alias)) {
        throw std::logic_error("Unexpected subcommand name: " + alias);
    }
    if (descriptions.find(alias) != descriptions.end()) {
        throw std::logic_error("Alias described twice: " + alias);
    }

    descriptions.emplace(alias, descr);
}

void
SubCommand::error()
{
    hasErrors = true;
}

bool
SubCommand::isAlias(const std::string &alias) const
{
    // Check that name matches one of aliases.
    return (std::find(names.cbegin(), names.cend(), alias) != names.cend());
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
