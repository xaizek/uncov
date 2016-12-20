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

#include "Catch/catch.hpp"

#include <cstdlib>

#include <iostream>
#include <stdexcept>
#include <string>

#include "BuildHistory.hpp"
#include "DB.hpp"
#include "Repository.hpp"
#include "SubCommand.hpp"

#include "TestUtils.hpp"

static SubCommand * getCmd(const std::string &name);

TEST_CASE("Diff fails on wrong file path", "[subcommands][diff-subcommand]")
{
    Repository repo("tests/test-repo/subdir");
    DB db(repo.getGitPath() + "/uncover.sqlite");
    BuildHistory bh(db);

    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
    REQUIRE(getCmd("diff")->exec(bh, repo, { "no-such-path" }) == EXIT_FAILURE);
    CHECK(coutCapture.get() == std::string());
    CHECK(cerrCapture.get() != std::string());
}

static SubCommand *
getCmd(const std::string &name)
{
    for (SubCommand *cmd : SubCommand::getAll()) {
        if (cmd->getName() == name) {
            return cmd;
        }
    }
    throw std::invalid_argument("No such command: " + name);
}
