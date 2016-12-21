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

/**
 * @brief Temporarily redirects specified stream from a string.
 */
class StreamSubstitute
{
public:
    /**
     * @brief Constructs instance that redirects @p is.
     *
     * @param is Stream to redirect.
     * @param str Contents to splice into the stream.
     */
    StreamSubstitute(std::istream &is, const std::string &str)
        : is(is), iss(str)
    {
        rdbuf = is.rdbuf();
        is.rdbuf(iss.rdbuf());
    }

    /**
     * @brief Restores original state of the stream.
     */
    ~StreamSubstitute()
    {
        is.rdbuf(rdbuf);
    }

private:
    /**
     * @brief Stream that is being redirected.
     */
    std::istream &is;
    /**
     * @brief Temporary input buffer of the stream.
     */
    std::istringstream iss;
    /**
     * @brief Original input buffer of the stream.
     */
    std::streambuf *rdbuf;
};

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

TEST_CASE("New handles input gracefully", "[subcommands][new-subcommand]")
{
    Repository repo("tests/test-repo/subdir");
    DB db(repo.getGitPath() + "/uncover.sqlite");
    BuildHistory bh(db);
    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);

    SECTION("Missing hashsum")
    {
        StreamSubstitute cinSubst(std::cin,
                                  "8e354da4df664b71e06c764feb29a20d64351a01\n"
                                  "master\n"
                                  "test-file1.cpp\n"
                                  "5\n"
                                  "-1 1 -1 1 -1\n");
        REQUIRE(getCmd("new")->exec(bh, repo, {}) == EXIT_FAILURE);
    }

    SECTION("Not number in coverage")
    {
        StreamSubstitute cinSubst(std::cin,
                                  "8e354da4df664b71e06c764feb29a20d64351a01\n"
                                  "master\n"
                                  "test-file1.cpp\n"
                                  "7e734c598d6ebdc19bbd660f6a7a6c73\n"
                                  "5\n"
                                  "-1 asdf -1 1 -1\n");
        REQUIRE(getCmd("new")->exec(bh, repo, {}) == EXIT_FAILURE);
    }

    SECTION("Wrong file hash")
    {
        StreamSubstitute cinSubst(std::cin,
                                  "8e354da4df664b71e06c764feb29a20d64351a01\n"
                                  "master\n"
                                  "test-file1.cpp\n"
                                  "734c598d6ebdc19bbd660f6a7a6c73\n"
                                  "5\n"
                                  "-1 1 -1 1 -1\n");
        REQUIRE(getCmd("new")->exec(bh, repo, {}) == EXIT_FAILURE);
    }

    CHECK(coutCapture.get() == std::string());
    CHECK(cerrCapture.get() != std::string());
}

TEST_CASE("New creates new builds", "[subcommands][new-subcommand]")
{
    Repository repo("tests/test-repo/subdir");
    const std::string dbPath = repo.getGitPath() + "/uncover.sqlite";
    FileRestorer databaseRestorer(dbPath, dbPath + "_original");
    DB db(dbPath);
    BuildHistory bh(db);
    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);

    SECTION("File missing from commit is just skipped")
    {
        auto sizeWas = bh.getBuilds().size();
        StreamSubstitute cinSubst(std::cin,
                                  "8e354da4df664b71e06c764feb29a20d64351a01\n"
                                  "master\n"
                                  "no-such-file\n"
                                  "7e734c598d6ebdc19bbd660f6a7a6c73\n"
                                  "5\n"
                                  "-1 1 -1 1 -1\n");
        REQUIRE(getCmd("new")->exec(bh, repo, {}) == EXIT_SUCCESS);
        REQUIRE(bh.getBuilds().size() == sizeWas + 1);
        REQUIRE(bh.getBuilds().back().getPaths() == vs({}));

        CHECK(cerrCapture.get() != std::string());
    }

    SECTION("Well-formed input is accepted")
    {
        auto sizeWas = bh.getBuilds().size();
        StreamSubstitute cinSubst(std::cin,
                                  "8e354da4df664b71e06c764feb29a20d64351a01\n"
                                  "master\n"
                                  "test-file1.cpp\n"
                                  "7e734c598d6ebdc19bbd660f6a7a6c73\n"
                                  "5\n"
                                  "-1 1 -1 1 -1\n");
        REQUIRE(getCmd("new")->exec(bh, repo, {}) == EXIT_SUCCESS);
        REQUIRE(bh.getBuilds().size() == sizeWas + 1);
        REQUIRE(bh.getBuilds().back().getPaths() != vs({}));

        CHECK(cerrCapture.get() == std::string());
    }

    CHECK(coutCapture.get() != std::string());
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
