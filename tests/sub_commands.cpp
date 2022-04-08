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

#include "Catch/catch.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/optional.hpp>
#include <boost/scope_exit.hpp>

#include <cstdlib>

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#include "utils/strings.hpp"
#include "BuildHistory.hpp"
#include "DB.hpp"
#include "GcovImporter.hpp"
#include "Repository.hpp"
#include "SubCommand.hpp"
#include "Uncov.hpp"
#include "integration.hpp"

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

const std::string build3info =
R"(Id:                  #3                                      
Coverage:            100.00%                                 
C/R Lines:           2 / 2                                   
Cov Change:          +50.0000%                               
C/M/R Line Changes:  0 / -2 / -2                             
Ref:                 master                                  
Commit:              d1b12454989580b470be93e71cc60c2e32fd5889
Time:                2017-01-09 13:17:51                     
)";

const std::string allBuilds =
R"(BUILD  COVERAGE  C/R LINES  COV CHANGE  C/M/R LINE CHANGES     REF
   #1    50.00%      2 / 4   -50.0000%    +2 /   +2 /   +4  master
   #2    50.00%      2 / 4     0.0000%     0 /    0 /    0  master
   #3   100.00%      2 / 2   +50.0000%     0 /   -2 /   -2  master
)";

TEST_CASE("Error on too few args", "[subcommands]")
{
    Repository repo("tests/test-repo/subdir");
    DB db(getDbPath(repo));
    BuildHistory bh(db);

    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
    CHECK(getCmd("get")->exec(getSettings(), bh, repo, "get",
                              { "@10" }) == EXIT_FAILURE);

    CHECK(coutCapture.get() == "Too few subcommand arguments: 1.  "
                               "Expected exactly 2.\n");
    CHECK(cerrCapture.get() == std::string());
}

TEST_CASE("Error on too many args", "[subcommands]")
{
    Repository repo("tests/test-repo/subdir");
    DB db(getDbPath(repo));
    BuildHistory bh(db);

    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
    CHECK(getCmd("show")->exec(getSettings(), bh, repo, "show",
                               { "@10", "a/", "b" }) == EXIT_FAILURE);

    CHECK(coutCapture.get() == "Too many subcommand arguments: 3.  "
                               "Expected at least 0 and at most 2.\n");
    CHECK(cerrCapture.get() == std::string());
}

TEST_CASE("Error on wrong alias", "[subcommands]")
{
    Repository repo("tests/test-repo/subdir");
    DB db(getDbPath(repo));
    BuildHistory bh(db);

    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
    CHECK(getCmd("show")->exec(getSettings(), bh, repo, "sh",
                               { }) == EXIT_FAILURE);

    CHECK(coutCapture.get() == "Unexpected subcommand name: sh\n");
    CHECK(cerrCapture.get() == std::string());
}

TEST_CASE("Description can be queried", "[subcommands]")
{
    Repository repo("tests/test-repo/subdir");
    DB db(getDbPath(repo));
    BuildHistory bh(db);

    SubCommand *cmd = getCmd("show");
    CHECK(cmd->getDescription("show") == "Displays a build, directory or file");
    CHECK(cmd->getDescription("missed") == "Displays missed in a build, "
                                           "directory or file");
    CHECK_THROWS_AS(cmd->getDescription("wrong"), const std::out_of_range &);
}

TEST_CASE("Error on wrong branch", "[subcommands][build-subcommand]")
{
    Repository repo("tests/test-repo/subdir");
    DB db(getDbPath(repo));
    BuildHistory bh(db);

    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
    CHECK_THROWS_AS(getCmd("build")->exec(getSettings(), bh, repo, "build",
                                          { "@branch" }),
                    const std::runtime_error &);
}

TEST_CASE("Invalid arguments for build", "[subcommands][build-subcommand]")
{
    Repository repo("tests/test-repo/subdir");
    DB db(getDbPath(repo));
    BuildHistory bh(db);

    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
    CHECK(getCmd("build")->exec(getSettings(), bh, repo, "build",
                                { "something" }) == EXIT_FAILURE);

    CHECK(coutCapture.get() == std::string());
    CHECK(cerrCapture.get() == "Failed to parse arguments for `build`.\n"
                               "Valid invocation forms:\n"
                               " * uncov build [<build>]\n");
}

TEST_CASE("Build information on last build", "[subcommands][build-subcommand]")
{
    Repository repo("tests/test-repo/subdir");
    DB db(getDbPath(repo));
    BuildHistory bh(db);

    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
    CHECK(getCmd("build")->exec(getSettings(), bh, repo, "build",
                                { }) == EXIT_SUCCESS);

    CHECK(coutCapture.get() == build3info);
    CHECK(cerrCapture.get() == std::string());
}

TEST_CASE("Build information on a branch", "[subcommands][build-subcommand]")
{
    Repository repo("tests/test-repo/subdir");
    DB db(getDbPath(repo));
    BuildHistory bh(db);

    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
    CHECK(getCmd("build")->exec(getSettings(), bh, repo, "build",
                                { "@master" }) == EXIT_SUCCESS);

    CHECK(coutCapture.get() == build3info);
    CHECK(cerrCapture.get() == std::string());
}

TEST_CASE("Invalid arguments for builds", "[subcommands][builds-subcommand]")
{
    Repository repo("tests/test-repo/subdir");
    DB db(getDbPath(repo));
    BuildHistory bh(db);

    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
    CHECK(getCmd("builds")->exec(getSettings(), bh, repo, "builds",
                                 { "wrong" }) == EXIT_FAILURE);

    CHECK(coutCapture.get() == std::string());
    CHECK(cerrCapture.get() == "Failed to parse arguments for `builds`.\n"
                               "Valid invocation forms:\n"
                               " * uncov builds\n"
                               " * uncov builds <positive-num>\n"
                               " * uncov builds \"all\"\n");
}

TEST_CASE("Builds generates table", "[subcommands][builds-subcommand]")
{
    Repository repo("tests/test-repo/subdir");
    DB db(getDbPath(repo));
    BuildHistory bh(db);

    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
    CHECK(getCmd("builds")->exec(getSettings(), bh, repo, "builds",
                                 {}) == EXIT_SUCCESS);

    CHECK(coutCapture.get() == allBuilds);
    CHECK(cerrCapture.get() == std::string());
}

TEST_CASE("Builds generates full table", "[subcommands][builds-subcommand]")
{
    Repository repo("tests/test-repo/subdir");
    DB db(getDbPath(repo));
    BuildHistory bh(db);

    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
    CHECK(getCmd("builds")->exec(getSettings(), bh, repo, "builds",
                                 { "all" }) == EXIT_SUCCESS);

    CHECK(coutCapture.get() == allBuilds);
    CHECK(cerrCapture.get() == std::string());
}

TEST_CASE("Builds generates limited table", "[subcommands][builds-subcommand]")
{
    Repository repo("tests/test-repo/subdir");
    DB db(getDbPath(repo));
    BuildHistory bh(db);

    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
    CHECK(getCmd("builds")->exec(getSettings(), bh, repo, "builds",
                                 { "2" }) == EXIT_SUCCESS);

    const std::string expectedOut =
R"(BUILD  COVERAGE  C/R LINES  COV CHANGE  C/M/R LINE CHANGES     REF
   #2    50.00%      2 / 4     0.0000%     0 /    0 /    0  master
   #3   100.00%      2 / 2   +50.0000%     0 /   -2 /   -2  master
)";
    CHECK(coutCapture.get() == expectedOut);
    CHECK(cerrCapture.get() == std::string());
}

TEST_CASE("Diff fails on wrong file path", "[subcommands][diff-subcommand]")
{
    Repository repo("tests/test-repo/subdir");
    DB db(getDbPath(repo));
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
    DB db(getDbPath(repo));
    BuildHistory bh(db);

    CHECK_THROWS_AS(getCmd("diff")->exec(getSettings(), bh, repo, "diff",
                                         { "@-100" }),
                    const std::runtime_error &);
}

TEST_CASE("Diff and negative ids work", "[subcommands][diff-subcommand]")
{
    Repository repo("tests/test-repo/subdir");
    DB db(getDbPath(repo));
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
    DB db(getDbPath(repo));
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

TEST_CASE("Invalid arguments for get", "[subcommands][get-subcommand]")
{
    Repository repo("tests/test-repo/subdir");
    DB db(getDbPath(repo));
    BuildHistory bh(db);

    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
    CHECK(getCmd("get")->exec(getSettings(), bh, repo, "get",
                              { "/a", "/b" }) == EXIT_FAILURE);

    CHECK(coutCapture.get() == std::string());
    CHECK(cerrCapture.get() == "Failed to parse arguments for `get`.\n"
                               "Valid invocation forms:\n"
                               " * uncov get <build> <path>\n");
}

TEST_CASE("Paths to files can be relative inside repository",
          "[subcommands][get-subcommand]")
{
    Repository repo("tests/test-repo");
    DB db(getDbPath(repo));
    BuildHistory bh(db);

    Chdir chdirInsideRepoSubdir("tests/test-repo/subdir");

    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);

    SECTION("Relative within repository")
    {
        CHECK(getCmd("get")->exec(getSettings(), bh, repo, "get",
                                  { "@@", "../test-file1.cpp" }) ==
              EXIT_SUCCESS);
    }
    SECTION("Relative outside repository")
    {
        CHECK(getCmd("get")->exec(getSettings(), bh, repo, "get",
                                  { "@@", "../../test-repo/test-file1.cpp" }) ==
              EXIT_SUCCESS);
    }

    CHECK(coutCapture.get() != std::string());
    CHECK(cerrCapture.get() == std::string());
}

TEST_CASE("New handles input gracefully", "[subcommands][new-subcommand]")
{
    Repository repo("tests/test-repo/");
    DB db(getDbPath(repo));
    BuildHistory bh(db);
    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);

    SECTION("Empty input")
    {
        StreamSubstitute cinSubst(std::cin, "");
        CHECK(getCmd("new")->exec(getSettings(), bh, repo, "new",
                                  {}) == EXIT_FAILURE);
    }

    SECTION("No reference name")
    {
        StreamSubstitute cinSubst(std::cin,
                                  "8e354da4df664b71e06c764feb29a20d64351a01\n");
        CHECK(getCmd("new")->exec(getSettings(), bh, repo, "new",
                                  {}) == EXIT_FAILURE);
    }

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
    const std::string dbPath = getDbPath(repo);
    FileRestorer databaseRestorer(dbPath, dbPath + "_original");
    DB db(dbPath);
    BuildHistory bh(db);
    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);

    SECTION("Reference name can contain spaces")
    {
        auto sizeWas = bh.getBuilds().size();
        StreamSubstitute cinSubst(std::cin,
                                  "8e354da4df664b71e06c764feb29a20d64351a01\n"
                                  "WIP on master\n");
        CHECK(getCmd("new")->exec(getSettings(), bh, repo, "new",
                                  {}) == EXIT_SUCCESS);
        REQUIRE(bh.getBuilds().size() == sizeWas + 1);
        REQUIRE(bh.getBuilds().back().getPaths() == vs({}));

        CHECK(cerrCapture.get() == std::string());
    }

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
    DB db(getDbPath(repo));
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
                        const std::exception &);
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
    const std::string dbPath = getDbPath(repo);
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
    DB db(getDbPath(repo));
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
    DB db(getDbPath(repo));
    BuildHistory bh(db);

    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
    CHECK(getCmd("files")->exec(getSettings(), bh, repo, "files",
                                { "no-such-path" }) == EXIT_FAILURE);
    CHECK(coutCapture.get() == std::string());
    CHECK(cerrCapture.get() != std::string());
}

TEST_CASE("Files lists files and resolves negative build ids",
          "[subcommands][files-subcommand]")
{
    Repository repo("tests/test-repo/subdir");
    DB db(getDbPath(repo));
    BuildHistory bh(db);

    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
    CHECK(getCmd("files")->exec(getSettings(), bh, repo, "files",
                                { "@-2", "@@" }) == EXIT_SUCCESS);

    const std::string expectedOut =
R"(FILE            COVERAGE  C/R LINES  COV CHANGE  C/M/R LINE CHANGES
test-file1.cpp   100.00%      0 / 0     0.0000%    -2 /    0 /   -2
test-file2.cpp   100.00%      2 / 2  +100.0000%    +2 /   -2 /    0
)";
    CHECK(coutCapture.get() == expectedOut);
    CHECK(cerrCapture.get() == std::string());
}

TEST_CASE("Dirs without arguments uses latest build",
          "[subcommands][dirs-subcommand]")
{
    Repository repo("tests/test-repo/subdir");
    DB db(getDbPath(repo));
    BuildHistory bh(db);

    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
    CHECK(getCmd("dirs")->exec(getSettings(), bh, repo, "dirs",
                               {}) == EXIT_SUCCESS);

    const std::string expectedOut =
R"(DIRECTORY  COVERAGE  C/R LINES  COV CHANGE  C/M/R LINE CHANGES
/           100.00%      2 / 2   +50.0000%     0 /   -2 /   -2
)";
    CHECK(coutCapture.get() == expectedOut);
    CHECK(cerrCapture.get() == std::string());
}

TEST_CASE("Changed lists files in specified build",
          "[subcommands][changed-subcommand]")
{
    Repository repo("tests/test-repo/subdir");
    DB db(getDbPath(repo));
    BuildHistory bh(db);

    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
    CHECK(getCmd("changed")->exec(getSettings(), bh, repo, "changed",
                                  { "@@", "/test-file1.cpp" }) == EXIT_SUCCESS);

    const std::string expectedOut =
R"(FILE            COVERAGE  C/R LINES  COV CHANGE  C/M/R LINE CHANGES
test-file1.cpp   100.00%      0 / 0  +100.0000%     0 /   -2 /   -2
)";
    CHECK(coutCapture.get() == expectedOut);
    CHECK(cerrCapture.get() == std::string());
}

TEST_CASE("Diff fails when given one buildid and path",
          "[subcommands][diff-subcommand]")
{
    Repository repo("tests/test-repo/subdir");
    DB db(getDbPath(repo));
    BuildHistory bh(db);

    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
    CHECK(getCmd("diff")->exec(getSettings(), bh, repo, "diff",
                               { "@@", "/test-file1.cpp" }) == EXIT_FAILURE);
    CHECK(coutCapture.get() == std::string());
    CHECK(cerrCapture.get() != std::string());
}

TEST_CASE("Invalid arguments for show", "[subcommands][show-subcommand]")
{
    Repository repo("tests/test-repo/subdir");
    DB db(getDbPath(repo));
    BuildHistory bh(db);

    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
    CHECK(getCmd("show")->exec(getSettings(), bh, repo, "show",
                               { "/a", "/b" }) == EXIT_FAILURE);

    CHECK(coutCapture.get() == std::string());
    CHECK(cerrCapture.get() == "Failed to parse arguments for `show`.\n"
                               "Valid invocation forms:\n"
                               " * uncov show [<build>]\n"
                               " * uncov show <path>\n"
                               " * uncov show <build> <path>\n");
}

TEST_CASE("Whole build is printed", "[subcommands][show-subcommand]")
{
    const std::string build =
R"(Build: #3, 100.00%(2/2), +50.0000%(0/  -2/  -2), master
-------------------------------------------------------------------------------
File: test-file1.cpp, 100.00%(0/0), +100.0000%(0/-2/-2)
-------------------------------------------------------------------------------
    1       : int
    2       : main(int argc, char *argv[])
    3       : {
    4       :         return 0;
    5       : }
-------------------------------------------------------------------------------
File: test-file2.cpp, 100.00%(2/2), 0.0000%(0/0/0)
-------------------------------------------------------------------------------
    1       : int
    2    x1 : main(int argc, char *argv[])
    3       : {
    4    x1 :         return 0;
    5       : }
)";

    Repository repo("tests/test-repo/subdir");
    DB db(getDbPath(repo));
    BuildHistory bh(db);

    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
    CHECK(getCmd("show")->exec(getSettings(), bh, repo, "show",
                               { }) == EXIT_SUCCESS);
    CHECK(coutCapture.get() == build);
    CHECK(cerrCapture.get() == std::string());
}

TEST_CASE("Whole directory is printed", "[subcommands][show-subcommand]")
{
    const std::string build =
R"(Build: #3, 100.00%(2/2), +50.0000%(0/  -2/  -2), master
-------------------------------------------------------------------------------
File: test-file1.cpp, 100.00%(0/0), +100.0000%(0/-2/-2)
-------------------------------------------------------------------------------
    1       : int
    2       : main(int argc, char *argv[])
    3       : {
    4       :         return 0;
    5       : }
-------------------------------------------------------------------------------
File: test-file2.cpp, 100.00%(2/2), 0.0000%(0/0/0)
-------------------------------------------------------------------------------
    1       : int
    2    x1 : main(int argc, char *argv[])
    3       : {
    4    x1 :         return 0;
    5       : }
)";

    Repository repo("tests/test-repo/subdir");
    DB db(getDbPath(repo));
    BuildHistory bh(db);

    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
    CHECK(getCmd("show")->exec(getSettings(), bh, repo, "show",
                               { "/" }) == EXIT_SUCCESS);
    CHECK(coutCapture.get() == build);
    CHECK(cerrCapture.get() == std::string());
}

TEST_CASE("Invalid file path is handled", "[subcommands][show-subcommand]")
{
    Repository repo("tests/test-repo/subdir");
    DB db(getDbPath(repo));
    BuildHistory bh(db);

    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
    CHECK(getCmd("show")->exec(getSettings(), bh, repo, "show",
                               { "test-file100.cpp" }) == EXIT_FAILURE);
    CHECK(coutCapture.get() == std::string());
    CHECK(cerrCapture.get() != std::string());
}

TEST_CASE("Specific file of latest build is printed",
          "[subcommands][show-subcommand]")
{
    const std::string file1 =
R"(Build: #3, 100.00%(2/2), +50.0000%(0/  -2/  -2), master
-------------------------------------------------------------------------------
File: test-file1.cpp, 100.00%(0/0), +100.0000%(0/-2/-2)
-------------------------------------------------------------------------------
    1       : int
    2       : main(int argc, char *argv[])
    3       : {
    4       :         return 0;
    5       : }
)";

    Repository repo("tests/test-repo/subdir");
    DB db(getDbPath(repo));
    BuildHistory bh(db);

    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
    CHECK(getCmd("show")->exec(getSettings(), bh, repo, "show",
                               { "test-file1.cpp" }) == EXIT_SUCCESS);
    CHECK(coutCapture.get() == file1);
    CHECK(cerrCapture.get() == std::string());
}

TEST_CASE("Specific file of specific build is printed",
          "[subcommands][show-subcommand]")
{
    const std::string file1 =
R"(Build: #2, 50.00%(2/4), 0.0000%(0/   0/   0), master
-------------------------------------------------------------------------------
File: test-file1.cpp, 0.00%(0/2), -100.0000%(-2/+2/0)
-------------------------------------------------------------------------------
    1       : int
    2    x0 : main(int argc, char *argv[])
    3       : {
    4    x0 :         return 0;
    5       : }
)";

    Repository repo("tests/test-repo/subdir");
    DB db(getDbPath(repo));
    BuildHistory bh(db);

    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
    CHECK(getCmd("show")->exec(getSettings(), bh, repo, "show",
                               { "@2", "test-file1.cpp" }) == EXIT_SUCCESS);
    CHECK(coutCapture.get() == file1);
    CHECK(cerrCapture.get() == std::string());
}

TEST_CASE("Nothing is printed for a completely covered file",
          "[subcommands][missed-subcommand]")
{
    const std::string file1 =
        "Build: #3, 100.00%(2/2), +50.0000%(0/  -2/  -2), master\n";

    Repository repo("tests/test-repo/subdir");
    DB db(getDbPath(repo));
    BuildHistory bh(db);

    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
    CHECK(getCmd("missed")->exec(getSettings(), bh, repo, "missed",
                                 { "test-file1.cpp" }) == EXIT_SUCCESS);
    CHECK(coutCapture.get() == file1);
    CHECK(cerrCapture.get() == std::string());
}

TEST_CASE("new-gcovi invokes gcov", "[subcommands][new-gcovi-subcommand]")
{
    // Remove destination directory if it exists to account for crashes.
    TempDirCopy tempDirCopy("tests/test-repo-gcno/_git",
                            "tests/test-repo-gcno/.git",
                            true);

    CHECK(queryProc({ "./test-repo-gcno" }, "tests/test-repo-gcno/",
                    CatchStderr{}) == EXIT_SUCCESS);

    Repository repo("tests/test-repo-gcno");
    const std::string dbPath = getDbPath(repo);
    BOOST_SCOPE_EXIT_ALL(dbPath) {
        std::remove(dbPath.c_str());
    };
    DB db(dbPath);
    BuildHistory bh(db);

    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
    CHECK(getCmd("new-gcovi")->exec(getSettings(), bh, repo, "new-gcovi",
                                    { "--verbose",
                                      "tests/test-repo-gcno" }) ==
          EXIT_SUCCESS);
    CHECK(split(coutCapture.get(), '\n').size() > 2U);
    CHECK(coutCapture.get() != std::string());
    CHECK(cerrCapture.get() == std::string());

    boost::optional<Build> build = bh.getBuild(1);
    REQUIRE(build);
    CHECK(build->getPaths().size() == 1U);
    CHECK(build->getCoveredCount() > 0);
    CHECK(build->getMissedCount() == 0);
}

TEST_CASE("Empty coverage data is imported",
          "[subcommands][new-gcovi-subcommand]")
{
    const std::string newBuildInfo =
        "Build: #4, 100.00%(0/0), 0.0000%(-2/   0/  -2), master\n";

    Repository repo("tests/test-repo");
    const std::string dbPath = getDbPath(repo);
    FileRestorer databaseRestorer(dbPath, dbPath + "_original");
    DB db(dbPath);
    BuildHistory bh(db);

    auto runner = [](std::vector<std::string> &&cmd,
                     const std::string &/*from*/) {
        REQUIRE(cmd.size() == 4U);
        return std::string();
    };
    GcovImporter::setRunner(runner);

    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
    CHECK(getCmd("new-gcovi")->exec(getSettings(), bh, repo, "new-gcovi",
                                    { "tests/test-repo" }) == EXIT_SUCCESS);
    CHECK(coutCapture.get() == newBuildInfo);
    CHECK(cerrCapture.get() == std::string());

    boost::optional<Build> build = bh.getBuild(4);
    REQUIRE(build);
    CHECK(build->getPaths().size() == 3U);
}

TEST_CASE("Coverage file is discovered",
          "[subcommands][new-gcovi-subcommand]")
{
    const std::string newBuildInfo =
        "Build: #4, 100.00%(0/0), 0.0000%(-2/   0/  -2), master\n";

    Repository repo("tests/test-repo");
    const std::string dbPath = getDbPath(repo);
    FileRestorer databaseRestorer(dbPath, dbPath + "_original");
    DB db(dbPath);
    BuildHistory bh(db);

    std::string gcnoFile = "tests/test-repo/test-file1.gcno";
    BOOST_SCOPE_EXIT_ALL(gcnoFile) {
        std::remove(gcnoFile.c_str());
    };
    std::ofstream{gcnoFile};

    auto runner = [&gcnoFile](std::vector<std::string> &&cmd,
                              const std::string &/*from*/) {
        REQUIRE(cmd.size() == 5U);
        CHECK(boost::ends_with(cmd[4], gcnoFile));
        return std::string();
    };
    GcovImporter::setRunner(runner);

    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
    CHECK(getCmd("new-gcovi")->exec(getSettings(), bh, repo, "new-gcovi",
                                    { "tests/test-repo" }) == EXIT_SUCCESS);
    CHECK(coutCapture.get() == newBuildInfo);
    CHECK(cerrCapture.get() == std::string());

    boost::optional<Build> build = bh.getBuild(4);
    REQUIRE(build);
    CHECK(build->getPaths().size() == 3U);
}

TEST_CASE("Unexecuted files can be excluded",
          "[subcommands][new-gcovi-subcommand]")
{
    const std::string newBuildInfo =
        "Build: #4, 100.00%(0/0), 0.0000%(-2/   0/  -2), master\n";

    Repository repo("tests/test-repo");
    const std::string dbPath = getDbPath(repo);
    FileRestorer databaseRestorer(dbPath, dbPath + "_original");
    DB db(dbPath);
    BuildHistory bh(db);

    auto runner = [](std::vector<std::string> &&/*cmd*/,
                     const std::string &/*from*/) {
        return std::string();
    };
    GcovImporter::setRunner(runner);

    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
    CHECK(getCmd("new-gcovi")->exec(getSettings(), bh, repo, "new-gcovi",
                                    { "--exclude", "subdir",
                                      "tests/test-repo" }) == EXIT_SUCCESS);
    CHECK(coutCapture.get() == newBuildInfo);
    CHECK(cerrCapture.get() == std::string());

    boost::optional<Build> build = bh.getBuild(4);
    REQUIRE(build);
    CHECK(build->getPaths().size() == 2U);
}

TEST_CASE("new-gcovi --prefix", "[subcommands][new-gcovi-subcommand]")
{
    const std::string newBuildInfo =
        "Build: #4, 100.00%(0/0), 0.0000%(-2/   0/  -2), master\n";

    Repository repo("tests/test-repo");
    const std::string dbPath = getDbPath(repo);
    FileRestorer databaseRestorer(dbPath, dbPath + "_original");
    DB db(dbPath);
    BuildHistory bh(db);

    auto runner = [](std::vector<std::string> &&/*cmd*/,
                     const std::string &from) {
        std::ofstream{from + "/subdir#file.gcov"}
            << "file:file.cpp\n";
        return std::string();
    };
    GcovImporter::setRunner(runner);

    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
    CHECK(getCmd("new-gcovi")->exec(getSettings(), bh, repo, "new-gcovi",
                                    { "--prefix=subdir",
                                      "tests/test-repo" }) == EXIT_SUCCESS);
    CHECK(coutCapture.get() == newBuildInfo);
    CHECK(cerrCapture.get() == std::string());

    boost::optional<Build> build = bh.getBuild(4);
    REQUIRE(build);
    CHECK(build->getPaths().size() == 3U);
}

TEST_CASE("Executed files can be excluded",
          "[subcommands][new-gcovi-subcommand]")
{
    const std::string newBuildInfo =
        "Build: #4, 100.00%(0/0), 0.0000%(-2/   0/  -2), master\n";

    Repository repo("tests/test-repo");
    const std::string dbPath = getDbPath(repo);
    FileRestorer databaseRestorer(dbPath, dbPath + "_original");
    DB db(dbPath);
    BuildHistory bh(db);

    auto runner = [](std::vector<std::string> &&/*cmd*/,
                     const std::string &from) {
        std::ofstream{from + "/subdir#file.gcov"}
            << "file:subdir/file.cpp\n";
        return std::string();
    };
    GcovImporter::setRunner(runner);

    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
    CHECK(getCmd("new-gcovi")->exec(getSettings(), bh, repo, "new-gcovi",
                                    { "--exclude", "subdir",
                                      "tests/test-repo" }) == EXIT_SUCCESS);
    CHECK(coutCapture.get() == newBuildInfo);
    CHECK(cerrCapture.get() == std::string());

    boost::optional<Build> build = bh.getBuild(4);
    REQUIRE(build);
    CHECK(build->getPaths().size() == 2U);
}

TEST_CASE("Gcov file is found and parsed",
          "[subcommands][new-gcovi-subcommand]")
{
    const std::string newBuildInfo =
        "Build: #4, 100.00%(2/2), 0.0000%(0/   0/   0), master\n";

    Repository repo("tests/test-repo");
    const std::string dbPath = getDbPath(repo);
    FileRestorer databaseRestorer(dbPath, dbPath + "_original");
    DB db(dbPath);
    BuildHistory bh(db);

    auto runner = [](std::vector<std::string> &&/*cmd*/,
                     const std::string &from) {
        std::string json = R"({
            "current_working_directory": "./",
            "files": [{
                "file": "test-file1.cpp",
                "lines": [
                    { "line_number": 2, "count": 1 },
                    { "line_number": 4, "count": 1 }
                ]
            }]
        })";

        GcovInfo gcovInfo;
        if (from == "-") {
            removeChars(json, '\n');
            return json;
        } else if (gcovInfo.hasJsonFormat()) {
            makeGz(from + "/test-file1.gcno.gcov.json.gz", json);
        } else {
            std::ofstream{from + "/test-file1.gcov"}
                << "file:test-file1.cpp\n"
                << "lcount:2,1\n"
                << "lcount:4,1\n";
        }

        return std::string();
    };
    GcovImporter::setRunner(runner);

    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
    CHECK(getCmd("new-gcovi")->exec(getSettings(), bh, repo, "new-gcovi",
                                    { "tests/test-repo" }) == EXIT_SUCCESS);
    CHECK(coutCapture.get() == newBuildInfo);
    CHECK(cerrCapture.get() == std::string());

    boost::optional<Build> build = bh.getBuild(4);
    REQUIRE(build);
    CHECK(build->getPaths().size() == 3U);
}

TEST_CASE("Gcov file with broken format causes an exception",
          "[subcommands][new-gcovi-subcommand]")
{
    Repository repo("tests/test-repo");
    const std::string dbPath = getDbPath(repo);
    FileRestorer databaseRestorer(dbPath, dbPath + "_original");
    DB db(dbPath);
    BuildHistory bh(db);

    auto runner = [](std::vector<std::string> &&/*cmd*/,
                     const std::string &from) {
        std::string json = R"({
            "files": [{
                "file": "test-file1.cpp",
                "lines": [ { "line_number": 2, "count": 0 }, ]
            }]
        })";

        GcovInfo gcovInfo;
        if (from == "-") {
            removeChars(json, '\n');
            return json;
        } else if (gcovInfo.hasJsonFormat()) {
            makeGz(from + "/test-file1.gcno.gcov.json.gz", json);
        } else {
            std::ofstream{from + "/test-file1.gcov"}
                << "file:test-file1.cpp\n"
                << "lcount:2\n";
        }

        return std::string();
    };
    GcovImporter::setRunner(runner);

    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
    CHECK_THROWS_AS(getCmd("new-gcovi")->exec(getSettings(), bh, repo,
                                              "new-gcovi",
                                              { "tests/test-repo" }),
                    const std::runtime_error &);
    CHECK(coutCapture.get() == std::string());
    CHECK(cerrCapture.get() == std::string());

    REQUIRE_FALSE(bh.getBuild(4));
}

TEST_CASE("Modified source file is captured",
          "[subcommands][new-gcovi-subcommand]")
{
    const std::string newBuildInfo =
        "Build: #4, 100.00%(0/0), 0.0000%(-2/   0/  -2), WIP on master\n";

    Repository repo("tests/test-repo");
    const std::string dbPath = getDbPath(repo);
    FileRestorer databaseRestorer(dbPath, dbPath + "_original");
    DB db(dbPath);
    BuildHistory bh(db);

    std::string sourceFile = "tests/test-repo/subdir/file.cpp";
    FileRestorer sourceFileRestorer(sourceFile, sourceFile + "_original");
    std::ofstream{sourceFile} << "int f() { return 666; }";

    auto runner = [](std::vector<std::string> &&/*cmd*/,
                     const std::string &/*from*/) {
        return std::string();
    };
    GcovImporter::setRunner(runner);

    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
    CHECK(getCmd("new-gcovi")->exec(getSettings(), bh, repo, "new-gcovi",
                                    { "--capture-worktree",
                                      "tests/test-repo" }) == EXIT_SUCCESS);
    CHECK(coutCapture.get() == newBuildInfo);
    CHECK(cerrCapture.get() == std::string());

    boost::optional<Build> build = bh.getBuild(4);
    REQUIRE(build);
    CHECK(build->getPaths().size() == 3U);
}

TEST_CASE("Untracked source file is captured",
          "[subcommands][new-gcovi-subcommand]")
{
    const std::string newBuildInfo =
        "Build: #4, 100.00%(0/0), 0.0000%(-2/   0/  -2), WIP on master\n";

    Repository repo("tests/test-repo");
    const std::string dbPath = getDbPath(repo);
    FileRestorer databaseRestorer(dbPath, dbPath + "_original");
    DB db(dbPath);
    BuildHistory bh(db);

    std::string untrackedFile = "tests/test-repo/test-file3.cpp";
    BOOST_SCOPE_EXIT_ALL(untrackedFile) {
        std::remove(untrackedFile.c_str());
    };
    std::ofstream{untrackedFile};

    auto runner = [](std::vector<std::string> &&/*cmd*/,
                     const std::string &/*from*/) {
        return std::string();
    };
    GcovImporter::setRunner(runner);

    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
    CHECK(getCmd("new-gcovi")->exec(getSettings(), bh, repo, "new-gcovi",
                                    { "--capture-worktree",
                                      "tests/test-repo" }) == EXIT_SUCCESS);
    CHECK(coutCapture.get() == newBuildInfo);
    CHECK(cerrCapture.get() == std::string());

    boost::optional<Build> build = bh.getBuild(4);
    REQUIRE(build);
    CHECK(build->getPaths().size() == 4U);
}

TEST_CASE("Untracked source file is rejected without capture",
          "[subcommands][new-gcovi-subcommand]")
{
    const std::string newBuildInfo =
        "Build: #4, 100.00%(0/0), 0.0000%(-2/   0/  -2), master\n";
    const std::string error =
        "Skipping file missing in master: test-file3.cpp\n";

    Repository repo("tests/test-repo");
    const std::string dbPath = getDbPath(repo);
    FileRestorer databaseRestorer(dbPath, dbPath + "_original");
    DB db(dbPath);
    BuildHistory bh(db);

    std::string untrackedFile = "tests/test-repo/test-file3.cpp";
    BOOST_SCOPE_EXIT_ALL(untrackedFile) {
        std::remove(untrackedFile.c_str());
    };
    std::ofstream{untrackedFile};

    auto runner = [](std::vector<std::string> &&/*cmd*/,
                     const std::string &/*from*/) {
        return std::string();
    };
    GcovImporter::setRunner(runner);

    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
    CHECK(getCmd("new-gcovi")->exec(getSettings(), bh, repo, "new-gcovi",
                                    { "tests/test-repo" }) == EXIT_SUCCESS);
    CHECK(coutCapture.get() == newBuildInfo);
    CHECK(cerrCapture.get() == error);

    boost::optional<Build> build = bh.getBuild(4);
    REQUIRE(build);
    CHECK(build->getPaths().size() == 3U);
}

TEST_CASE("Unmatched source fails build addition",
          "[subcommands][new-gcovi-subcommand]")
{
    const std::string error =
        "subdir/file.cpp file at master doesn't match computed MD5 hash\n";

    Repository repo("tests/test-repo");
    const std::string dbPath = getDbPath(repo);
    FileRestorer databaseRestorer(dbPath, dbPath + "_original");
    DB db(dbPath);
    BuildHistory bh(db);

    std::string sourceFile = "tests/test-repo/subdir/file.cpp";
    FileRestorer sourceFileRestorer(sourceFile, sourceFile + "_original");
    std::ofstream{sourceFile} << "int f() { return 666; }";

    auto runner = [](std::vector<std::string> &&/*cmd*/,
                     const std::string &/*from*/) {
        return std::string();
    };
    GcovImporter::setRunner(runner);

    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
    CHECK(getCmd("new-gcovi")->exec(getSettings(), bh, repo, "new-gcovi",
                                    { "tests/test-repo" }) == EXIT_FAILURE);
    CHECK(coutCapture.get() == std::string());
    CHECK(cerrCapture.get() == error);

    REQUIRE_FALSE(bh.getBuild(4));
}

TEST_CASE("new-gcovi --help", "[subcommands][new-gcovi-subcommand]")
{
    Repository repo("tests/test-repo");
    const std::string dbPath = getDbPath(repo);
    FileRestorer databaseRestorer(dbPath, dbPath + "_original");
    DB db(dbPath);
    BuildHistory bh(db);

    auto runner = [](std::vector<std::string> &&/*cmd*/,
                     const std::string &/*from*/) {
        return std::string();
    };
    GcovImporter::setRunner(runner);

    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
    CHECK(getCmd("new-gcovi")->exec(getSettings(), bh, repo, "new-gcovi",
                                    { "--help",
                                      "tests/test-repo" }) == EXIT_SUCCESS);
    CHECK(coutCapture.get() != std::string());
    CHECK(cerrCapture.get() == std::string());

    REQUIRE_FALSE(bh.getBuild(4));
}

TEST_CASE("new-gcovi --ref-name", "[subcommands][new-gcovi-subcommand]")
{
    const std::string newBuildInfo =
        "Build: #4, 100.00%(0/0), 0.0000%(-2/   0/  -2), test-ref-name\n";

    Repository repo("tests/test-repo");
    const std::string dbPath = getDbPath(repo);
    FileRestorer databaseRestorer(dbPath, dbPath + "_original");
    DB db(dbPath);
    BuildHistory bh(db);

    auto runner = [](std::vector<std::string> &&/*cmd*/,
                     const std::string &/*from*/) {
        return std::string();
    };
    GcovImporter::setRunner(runner);

    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
    CHECK(getCmd("new-gcovi")->exec(getSettings(), bh, repo, "new-gcovi",
                                    { "--ref-name", "test-ref-name",
                                      "tests/test-repo" }) == EXIT_SUCCESS);
    CHECK(coutCapture.get() == newBuildInfo);
    CHECK(cerrCapture.get() == std::string());

    boost::optional<Build> build = bh.getBuild(4);
    REQUIRE(build);
    CHECK(build->getRefName() == "test-ref-name");
}

TEST_CASE("new-gcovi --capture-worktree can be noop",
          "[subcommands][new-gcovi-subcommand]")
{
    const std::string newBuildInfo =
        "Build: #4, 100.00%(0/0), 0.0000%(-2/   0/  -2), WIP on master\n";

    Repository repo("tests/test-repo");
    const std::string dbPath = getDbPath(repo);
    FileRestorer databaseRestorer(dbPath, dbPath + "_original");
    DB db(dbPath);
    BuildHistory bh(db);

    auto runner = [](std::vector<std::string> &&/*cmd*/,
                     const std::string &/*from*/) {
        return std::string();
    };
    GcovImporter::setRunner(runner);

    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
    CHECK(getCmd("new-gcovi")->exec(getSettings(), bh, repo, "new-gcovi",
                                    { "--capture-worktree",
                                      "tests/test-repo" }) == EXIT_SUCCESS);
    CHECK(coutCapture.get() != std::string());
    CHECK(cerrCapture.get() == std::string());

    boost::optional<Build> build = bh.getBuild(4);
    REQUIRE(build);
    CHECK(build->getPaths().size() == 3U);
}

TEST_CASE("new-gcovi --verbose", "[subcommands][new-gcovi-subcommand]")
{
    Repository repo("tests/test-repo");
    const std::string dbPath = getDbPath(repo);
    FileRestorer databaseRestorer(dbPath, dbPath + "_original");
    DB db(dbPath);
    BuildHistory bh(db);

    std::string untrackedFile = "tests/test-repo/test-file3.cpp";
    BOOST_SCOPE_EXIT_ALL(untrackedFile) {
        std::remove(untrackedFile.c_str());
    };
    std::ofstream{untrackedFile};

    auto runner = [](std::vector<std::string> &&/*cmd*/,
                     const std::string &/*from*/) {
        return std::string();
    };
    GcovImporter::setRunner(runner);

    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
    CHECK(getCmd("new-gcovi")->exec(getSettings(), bh, repo, "new-gcovi",
                                    { "--verbose", "--capture-worktree",
                                      "tests/test-repo" }) == EXIT_SUCCESS);
    CHECK(split(coutCapture.get(), '\n').size() > 2U);
    CHECK(cerrCapture.get() == std::string());

    boost::optional<Build> build = bh.getBuild(4);
    REQUIRE(build);
    CHECK(build->getPaths().size() == 4U);
}

TEST_CASE("Generic help", "[subcommands][help-subcommand]")
{
    Repository repo("tests/test-repo");
    DB db(getDbPath(repo));
    BuildHistory bh(db);

    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
    Uncov uncov({ "uncov", "help" });
    CHECK(getCmd("help")->exec(uncov, "help", { }) == EXIT_SUCCESS);
    CHECK(boost::starts_with(coutCapture.get(),
                             "Usage: uncov [--help|-h] [--version|-v] [repo] "
                                    "subcommand [args...]\n"
                             "\n"
                             "Subcommands\n"));
    CHECK(cerrCapture.get() == std::string());
}

TEST_CASE("Specific help", "[subcommands][help-subcommand]")
{
    Repository repo("tests/test-repo");
    DB db(getDbPath(repo));
    BuildHistory bh(db);

    SECTION("Correct command")
    {
        StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
        Uncov uncov({ "uncov", "help", "new-gcovi" });
        CHECK(getCmd("help")->exec(uncov, "help",
                                   { "new-gcovi" }) == EXIT_SUCCESS);
        CHECK(boost::starts_with(coutCapture.get(),
                                 "new-gcovi\n\n"
                                 "Generates coverage via gcov and imports it\n"
                                 "\n"
                                 "Usage: uncov new-gcovi [options...] "
                                        "[covoutroot]\n"));
        CHECK(cerrCapture.get() == std::string());
    }

    SECTION("Unknown command")
    {
        StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);
        Uncov uncov({ "uncov", "help", "wrong-command" });
        CHECK_THROWS_AS(getCmd("help")->exec(uncov, "help",
                                             { "wrong-command" }),
                        const std::invalid_argument &);
    }
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
