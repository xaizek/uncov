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

#include <boost/algorithm/string/predicate.hpp>

#include <cstdlib>

#include <iostream>

#include "Uncov.hpp"

#include "TestUtils.hpp"

TEST_CASE("Help is printed", "[Uncov]")
{
    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);

    Uncov uncov({ "uncov", "--help" });
    CHECK(uncov.run(getSettings()) == EXIT_SUCCESS);

    CHECK(boost::starts_with(coutCapture.get(),
                             "Usage: uncov [--help|-h] [--version|-v] [repo] "
                                    "subcommand [args...]\n"
                             "\n"
                             "Subcommands\n"));
    CHECK(cerrCapture.get() == std::string());
}

TEST_CASE("Version is printed", "[Uncov]")
{
    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);

    Uncov uncov({ "uncov", "--version" });
    CHECK(uncov.run(getSettings()) == EXIT_SUCCESS);

    CHECK(boost::starts_with(coutCapture.get(), "uncov v"));
    CHECK(cerrCapture.get() == std::string());
}

TEST_CASE("Option error is handled well", "[Uncov]")
{
    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);

    Uncov uncov({ "uncov", "--bla" });
    CHECK(uncov.run(getSettings()) == EXIT_FAILURE);

    CHECK(coutCapture.get() == std::string());
    CHECK(cerrCapture.get() == "Usage error: unrecognised option '--bla'\n"
                               "\n"
                               "Usage: uncov [--help|-h] [--version|-v] [repo] "
                                      "subcommand [args...]\n");
}

TEST_CASE("Unknown command is handled well", "[Uncov]")
{
    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);

    Uncov uncov({ "uncov", "bla" });
    CHECK(uncov.run(getSettings()) == EXIT_FAILURE);

    CHECK(coutCapture.get() == std::string());
    CHECK(cerrCapture.get() == "Unknown subcommand: bla\n");
}

TEST_CASE("Command is executed", "[Uncov]")
{
    StreamCapture coutCapture(std::cout), cerrCapture(std::cerr);

    Uncov uncov({ "uncov", "tests/test-repo/_git/",
                  "get", "@1", "test-file1.cpp" });
    CHECK(uncov.run(getSettings()) == EXIT_SUCCESS);

    CHECK(coutCapture.get() == "8e354da4df664b71e06c764feb29a20d64351a01\n"
                               "-1\n1\n-1\n1\n-1\n");
    CHECK(cerrCapture.get() == std::string());
}
