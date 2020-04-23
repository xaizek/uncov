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

#include "Catch/catch.hpp"

#include <iostream>
#include <stdexcept>

#include "BuildHistory.hpp"
#include "DB.hpp"
#include "Repository.hpp"
#include "SubCommand.hpp"
#include "Uncov.hpp"

#include "TestUtils.hpp"

class TestSubCommand : public SubCommand
{
public:
    using SubCommand::SubCommand;
    using SubCommand::describe;

    virtual bool
    isGeneric() const override
    {
        return generic;
    }

    virtual void execImpl(const std::string &/*alias*/,
                          const std::vector<std::string> &/*args*/) override
    { }

    virtual void printHelp(std::ostream &/*os*/,
                           const std::string &/*alias*/) const override
    { }

    bool generic = false;
};

TEST_CASE("Description for unregistered alias is rejected", "[SubCommand]")
{
    TestSubCommand cmd({ "name" });
    CHECK_NOTHROW(cmd.describe("name", "descr"));
    CHECK_THROWS_AS(cmd.describe("not-an-alias", "descr"), std::logic_error);
}

TEST_CASE("Alias can be described only once", "[SubCommand]")
{
    TestSubCommand cmd({ "name" });
    CHECK_NOTHROW(cmd.describe("name", "descr"));
    CHECK_THROWS_AS(cmd.describe("name", "descr"), std::logic_error);
}

TEST_CASE("Repo command must be called using repo method", "[SubCommand]")
{
    Uncov uncov({ "uncov", "repo" });

    TestSubCommand cmd({ "repo" });
    cmd.describe("repo", "descr");
    cmd.generic = false;

    CHECK_THROWS_AS(cmd.exec(uncov, "repo", { }), std::logic_error);
}

TEST_CASE("Generic command must be called using generic method", "[SubCommand]")
{
    Repository repo("tests/test-repo/subdir");
    DB db(repo.getGitPath() + "/uncov.sqlite");
    BuildHistory bh(db);

    TestSubCommand cmd({ "generic" });
    cmd.describe("generic", "descr");
    cmd.generic = true;

    CHECK_THROWS_AS(cmd.exec(getSettings(), bh, repo, "generic", { }),
                    std::logic_error);
}
