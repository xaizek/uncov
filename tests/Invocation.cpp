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

#include <boost/algorithm/string/predicate.hpp>

#include <stdexcept>

#include "Invocation.hpp"

#include "TestUtils.hpp"

TEST_CASE("Constructor throws on empty argument list", "[Invocation]")
{
    REQUIRE_THROWS_AS(Invocation invocation({}), std::invalid_argument);
}

TEST_CASE("Invocation errors on too few arguments", "[Invocation]")
{
    SECTION("Program name only")
    {
        REQUIRE(Invocation({ "uncov" }).getError() != std::string());
    }

    SECTION("Program name and repository")
    {
        Invocation invocation({ "uncov", "." });
        REQUIRE(invocation.getError() != std::string());
    }
}

TEST_CASE("Well-formed command-line is parsed correctly", "[Invocation]")
{
    Invocation invocation({ "uncov", ".", "show", "arg1", "arg2" });
    REQUIRE(invocation.getError() == std::string());
    CHECK(invocation.getRepositoryPath() == ".");
    CHECK(invocation.getSubcommandName() == "show");
    CHECK(invocation.getSubcommandArgs() == vs({ "arg1", "arg2" }));
}

TEST_CASE("Repository argument is optional", "[Invocation]")
{
    SECTION("No repo argument")
    {
        Invocation invocation({ "uncov", "show", "arg1", "arg2" });
        REQUIRE(invocation.getError() == std::string());
        CHECK(invocation.getRepositoryPath() == ".");
        CHECK(invocation.getSubcommandName() == "show");
        CHECK(invocation.getSubcommandArgs() == vs({ "arg1", "arg2" }));
    }

    SECTION("Path with slash")
    {
        Invocation invocation({ "uncov", "a/path", "show", "arg1", "arg2" });
        REQUIRE(invocation.getError() == std::string());
        CHECK(invocation.getRepositoryPath() == "a/path");
        CHECK(invocation.getSubcommandName() == "show");
        CHECK(invocation.getSubcommandArgs() == vs({ "arg1", "arg2" }));
    }

    SECTION("Subcommand without arguments")
    {
        Invocation invocation({ "uncov", "builds" });
        REQUIRE(invocation.getError() == std::string());
        CHECK(invocation.getRepositoryPath() == ".");
        CHECK(invocation.getSubcommandName() == "builds");
        CHECK(invocation.getSubcommandArgs() == vs({}));
    }
}

TEST_CASE("Options are parsed", "[Invocation]")
{
    SECTION("Without repository name")
    {
        Invocation invocation({ "uncov", "--help", "--version" });
        REQUIRE(invocation.getError() == std::string());
        CHECK(invocation.shouldPrintHelp());
        CHECK(invocation.shouldPrintVersion());
    }

    SECTION("With repository name")
    {
        Invocation invocation({ "uncov", "--help", "." });
        REQUIRE(invocation.getError() == std::string());
        CHECK(invocation.shouldPrintHelp());
    }
}

TEST_CASE("Usage message includes program name", "[Invocation]")
{
    Invocation invocation({ "asdf", "subcommand" });

    CHECK(boost::contains(invocation.getUsage(), "asdf"));
}

TEST_CASE("Wrong option causes an error", "[Invocation]")
{
    Invocation invocation({ "uncov", "--no-such-option" });

    CHECK(invocation.getError() != std::string());
}
