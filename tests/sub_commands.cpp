// Copyright (C) 2016 xaizek <xaizek@posteo.net>
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

#include <boost/filesystem/operations.hpp>

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

    StreamSubstitute(const StreamSubstitute &rhs) = delete;
    StreamSubstitute & operator=(const StreamSubstitute &rhs) = delete;

    /**
     * @brief Restores original state of the stream.
     */
    ~StreamSubstitute()
    {
        is.rdbuf(rdbuf);
    }

private:
    //! Stream that is being redirected.
    std::istream &is;
    //! Temporary input buffer of the stream.
    std::istringstream iss;
    //! Original input buffer of the stream.
    std::streambuf *rdbuf;
};

class Chdir
{
public:
    explicit Chdir(const std::string &where)
        : previousPath(boost::filesystem::current_path())
    {
        boost::filesystem::current_path(where);
    }

    Chdir(const Chdir &rhs) = delete;
    Chdir & operator=(const Chdir &rhs) = delete;

    ~Chdir()
    {
        boost::system::error_code ec;
        boost::filesystem::current_path(previousPath, ec);
    }

private:
    const boost::filesystem::path previousPath;
};

static SubCommand * getCmd(const std::string &name);

TEST_CASE("Diff fails on wrong file path", "[subcommands][diff-subcommand]")
{
    Repository repo("tests/test-repo/subdir");
    DB db(repo.getGitPath() + "/uncov.sqlite");
    BuildHistory bh(db);

    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
    CHECK(getCmd("diff")->exec(getSettings(), bh, repo, "diff",
                               { "no-such-path" }) == EXIT_FAILURE);
    CHECK(coutCapture.get() == std::string());
    CHECK(cerrCapture.get() != std::string());
}

TEST_CASE("Diff fails on wrong negative id", "[subcommands][diff-subcommand]")
{
    Repository repo("tests/test-repo/subdir");
    DB db(repo.getGitPath() + "/uncov.sqlite");
    BuildHistory bh(db);

    CHECK_THROWS_AS(getCmd("diff")->exec(getSettings(), bh, repo, "diff",
                                         { "@-100" }),
                    std::runtime_error);
}

TEST_CASE("Diff and negative ids work", "[subcommands][diff-subcommand]")
{
    Repository repo("tests/test-repo/subdir");
    DB db(repo.getGitPath() + "/uncov.sqlite");
    BuildHistory bh(db);

    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
    CHECK(getCmd("diff")->exec(getSettings(), bh, repo, "diff",
                               { "@-2", "@-1" }) == EXIT_SUCCESS);

    const std::string expectedOut =
R"(-------------------------------------------------------------------------------
Build: #1, 50.00%(2/4), -50.0000%(+2/  +2/  +4), master
-------------------------------------------------------------------------------
Build: #2, 50.00%(2/4), 0.0000%(0/   0/   0), master
-------------------------------------------------------------------------------

-------------------------------------------------------------------------------
File: test-file1.cpp, 100.00% (2/2), 0.0000% (+2/0/+2)
-------------------------------------------------------------------------------
File: test-file1.cpp, 0.00% (0/2), -100.0000% (-2/+2/0)
-------------------------------------------------------------------------------
      :      : int
   x1 :   x0 : main(int argc, char *argv[])
      :      : {
   x1 :   x0 :         return 0;
      :      : }

-------------------------------------------------------------------------------
File: test-file2.cpp, 0.00% (0/2), -100.0000% (0/+2/+2)
-------------------------------------------------------------------------------
File: test-file2.cpp, 100.00% (2/2), +100.0000% (+2/-2/0)
-------------------------------------------------------------------------------
      :      : int
   x0 :   x1 : main(int argc, char *argv[])
      :      : {
   x0 :   x1 :         return 0;
      :      : }
)";
    CHECK(coutCapture.get() == expectedOut);
    CHECK(cerrCapture.get() == std::string());
}

TEST_CASE("Regress detects regression", "[subcommands][regress-subcommand]")
{
    Repository repo("tests/test-repo/subdir");
    DB db(repo.getGitPath() + "/uncov.sqlite");
    BuildHistory bh(db);

    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
    CHECK(getCmd("regress")->exec(getSettings(), bh, repo, "regress",
                                  { "@1", "@2" }) == EXIT_SUCCESS);

    const std::string expectedOut =
R"(-------------------------------------------------------------------------------
Build: #1, 50.00%(2/4), -50.0000%(+2/  +2/  +4), master
-------------------------------------------------------------------------------
Build: #2, 50.00%(2/4), 0.0000%(0/   0/   0), master
-------------------------------------------------------------------------------

-------------------------------------------------------------------------------
File: test-file1.cpp, 100.00% (2/2), 0.0000% (+2/0/+2)
-------------------------------------------------------------------------------
File: test-file1.cpp, 0.00% (0/2), -100.0000% (-2/+2/0)
-------------------------------------------------------------------------------
      :      : int
   x1 :   x0 : main(int argc, char *argv[])
      :      : {
   x1 :   x0 :         return 0;
      :      : }
)";
    CHECK(coutCapture.get() == expectedOut);
    CHECK(cerrCapture.get() == std::string());
}

TEST_CASE("Paths to files can be relative inside repository",
          "[subcommands][get-subcommand]")
{
    Repository repo("tests/test-repo");
    DB db(repo.getGitPath() + "/uncov.sqlite");
    BuildHistory bh(db);

    Chdir chdirInsideRepoSubdir("tests/test-repo/subdir");

    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
    CHECK(getCmd("get")->exec(getSettings(), bh, repo, "get",
                              { "@@", "../test-file1.cpp" }) == EXIT_SUCCESS);
    CHECK(coutCapture.get() != std::string());
    CHECK(cerrCapture.get() == std::string());
}

TEST_CASE("New handles input gracefully", "[subcommands][new-subcommand]")
{
    Repository repo("tests/test-repo/");
    DB db(repo.getGitPath() + "/uncov.sqlite");
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
        CHECK(getCmd("new")->exec(getSettings(), bh, repo, "new",
                                  {}) == EXIT_FAILURE);
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
        CHECK(getCmd("new")->exec(getSettings(), bh, repo, "new",
                                  {}) == EXIT_FAILURE);
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
        CHECK(getCmd("new")->exec(getSettings(), bh, repo, "new",
                                  {}) == EXIT_FAILURE);
    }

    CHECK(coutCapture.get() == std::string());
    CHECK(cerrCapture.get() != std::string());
}

TEST_CASE("New creates new builds", "[subcommands][new-subcommand]")
{
    Repository repo("tests/test-repo");
    const std::string dbPath = repo.getGitPath() + "/uncov.sqlite";
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
        CHECK(getCmd("new")->exec(getSettings(), bh, repo, "new",
                                  {}) == EXIT_SUCCESS);
        REQUIRE(bh.getBuilds().size() == sizeWas + 1);
        REQUIRE(bh.getBuilds().back().getPaths() == vs({}));

        CHECK(cerrCapture.get() != std::string());
    }

    SECTION("File path is normalized")
    {
        auto sizeWas = bh.getBuilds().size();
        StreamSubstitute cinSubst(std::cin,
                                  "8e354da4df664b71e06c764feb29a20d64351a01\n"
                                  "master\n"
                                  "././test-file1.cpp\n"
                                  "7e734c598d6ebdc19bbd660f6a7a6c73\n"
                                  "5\n"
                                  "-1 1 -1 1 -1\n");
        CHECK(getCmd("new")->exec(getSettings(), bh, repo, "new",
                                  {}) == EXIT_SUCCESS);
        REQUIRE(bh.getBuilds().size() == sizeWas + 1);
        REQUIRE(bh.getBuilds().back().getPaths() != vs({}));

        CHECK(cerrCapture.get() == std::string());
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
        CHECK(getCmd("new")->exec(getSettings(), bh, repo, "new",
                                  {}) == EXIT_SUCCESS);
        REQUIRE(bh.getBuilds().size() == sizeWas + 1);
        REQUIRE(bh.getBuilds().back().getPaths() != vs({}));

        CHECK(cerrCapture.get() == std::string());
    }

    CHECK(coutCapture.get() != std::string());
}

TEST_CASE("New-json handles input gracefully",
          "[subcommands][new-json-subcommand]")
{
    Repository repo("tests/test-repo/");
    DB db(repo.getGitPath() + "/uncov.sqlite");
    BuildHistory bh(db);
    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);

    SECTION("Missing hashsum")
    {
        StreamSubstitute cinSubst(std::cin, R"({
                "source_files": [
                    {
                        "name": "test-file1.cpp",
                        "coverage": [null, 1, null, 1, null]
                    }
                ],
                "git": {
                    "head": {
                        "id": "8e354da4df664b71e06c764feb29a20d64351a01"
                    },
                    "branch": "master"
                }
            })");

        CHECK_THROWS_AS(getCmd("new-json")->exec(getSettings(), bh, repo,
                                                 "new-json", {}),
                        std::exception);
        CHECK(cerrCapture.get() == std::string());
    }

    SECTION("Incorrect hashsum")
    {
        StreamSubstitute cinSubst(std::cin, R"({
                "source_files": [
                    {
                        "source_digest": "",
                        "name": "test-file1.cpp",
                        "coverage": [null, 1, null, 1, null]
                    }
                ],
                "git": {
                    "head": {
                        "id": "8e354da4df664b71e06c764feb29a20d64351a01"
                    },
                    "branch": "master"
                }
            })");

        CHECK(getCmd("new-json")->exec(getSettings(), bh, repo, "new-json",
                                       {}) == EXIT_FAILURE);
        CHECK(cerrCapture.get() != std::string());
    }

    CHECK(coutCapture.get() == std::string());
}

TEST_CASE("New-json creates new builds", "[subcommands][new-json-subcommand]")
{
    Repository repo("tests/test-repo");
    const std::string dbPath = repo.getGitPath() + "/uncov.sqlite";
    FileRestorer databaseRestorer(dbPath, dbPath + "_original");
    DB db(dbPath);
    BuildHistory bh(db);
    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);

    SECTION("File missing from commit is just skipped")
    {
        auto sizeWas = bh.getBuilds().size();
        StreamSubstitute cinSubst(std::cin, R"({
                "source_files": [
                    {
                        "source_digest":
                            "8e354da4df664b71e06c764feb29a20d64351a01",
                        "name": "not-a-file1.cpp",
                        "coverage": [null, 1, null, 1, null]
                    }
                ],
                "git": {
                    "head": {
                        "id": "8e354da4df664b71e06c764feb29a20d64351a01"
                    },
                    "branch": "master"
                }
            })");
        CHECK(getCmd("new-json")->exec(getSettings(), bh, repo, "new-json",
                                       {}) == EXIT_SUCCESS);
        CHECK(cerrCapture.get() != std::string());
        REQUIRE(bh.getBuilds().size() == sizeWas + 1);
        REQUIRE(bh.getBuilds().back().getPaths() == vs({}));
    }

    SECTION("JSON can be preceded by other data")
    {
        auto sizeWas = bh.getBuilds().size();
        StreamSubstitute cinSubst(std::cin, R"(something unrelated{
                "source_files": [],
                "git": {
                    "head": {
                        "id": "8e354da4df664b71e06c764feb29a20d64351a01"
                    },
                    "branch": "master"
                }
            })");
        CHECK(getCmd("new-json")->exec(getSettings(), bh, repo, "new-json",
                                       {}) == EXIT_SUCCESS);
        CHECK(cerrCapture.get() == std::string());
        REQUIRE(bh.getBuilds().size() == sizeWas + 1);
        REQUIRE(bh.getBuilds().back().getPaths() == vs({}));
    }

    SECTION("Hash is computed")
    {
        auto sizeWas = bh.getBuilds().size();
        StreamSubstitute cinSubst(std::cin, R"({
                "source_files": [
                    {
                        "source":
                 "int\nmain(int argc, char *argv[])\n{\n        return 0;\n}\n",
                        "name": "test-file1.cpp",
                        "coverage": [null, 1, null, 1, null]
                    }
                ],
                "git": {
                    "head": {
                        "id": "8e354da4df664b71e06c764feb29a20d64351a01"
                    },
                    "branch": "master"
                }
            })");
        CHECK(getCmd("new-json")->exec(getSettings(), bh, repo, "new-json",
                                       {}) == EXIT_SUCCESS);
        CHECK(cerrCapture.get() == std::string());
        REQUIRE(bh.getBuilds().size() == sizeWas + 1);
        REQUIRE(bh.getBuilds().back().getPaths() != vs({}));
    }

    SECTION("Newline is appended to contents")
    {
        auto sizeWas = bh.getBuilds().size();
        StreamSubstitute cinSubst(std::cin, R"({
                "source_files": [
                    {
                        "source":
                   "int\nmain(int argc, char *argv[])\n{\n        return 0;\n}",
                        "name": "test-file1.cpp",
                        "coverage": [null, 1, null, 1, null]
                    }
                ],
                "git": {
                    "head": {
                        "id": "8e354da4df664b71e06c764feb29a20d64351a01"
                    },
                    "branch": "master"
                }
            })");
        CHECK(getCmd("new-json")->exec(getSettings(), bh, repo, "new-json",
                                       {}) == EXIT_SUCCESS);
        CHECK(cerrCapture.get() == std::string());
        REQUIRE(bh.getBuilds().size() == sizeWas + 1);
        REQUIRE(bh.getBuilds().back().getPaths() != vs({}));
    }

    CHECK(coutCapture.get() != std::string());
}

TEST_CASE("Dirs fails on unknown dir path", "[subcommands][dirs-subcommand]")
{
    Repository repo("tests/test-repo/subdir");
    DB db(repo.getGitPath() + "/uncov.sqlite");
    BuildHistory bh(db);

    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
    CHECK(getCmd("dirs")->exec(getSettings(), bh, repo, "dirs",
                               { "no-such-path" }) == EXIT_FAILURE);
    CHECK(coutCapture.get() == std::string());
    CHECK(cerrCapture.get() != std::string());
}

TEST_CASE("Files fails on unknown dir path", "[subcommands][files-subcommand]")
{
    Repository repo("tests/test-repo/subdir");
    DB db(repo.getGitPath() + "/uncov.sqlite");
    BuildHistory bh(db);

    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
    CHECK(getCmd("files")->exec(getSettings(), bh, repo, "files",
                                { "no-such-path" }) == EXIT_FAILURE);
    CHECK(coutCapture.get() == std::string());
    CHECK(cerrCapture.get() != std::string());
}

static SubCommand *
getCmd(const std::string &name)
{
    for (SubCommand *cmd : SubCommand::getAll()) {
        for (const std::string &alias : cmd->getNames()) {
            if (alias == name) {
                return cmd;
            }
        }
    }
    throw std::invalid_argument("No such command: " + name);
}
