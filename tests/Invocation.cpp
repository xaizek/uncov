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
        REQUIRE(Invocation({ "uncover" }).getError() != std::string());
    }

    SECTION("Program name and repository")
    {
        Invocation invocation({ "uncover", "." });
        REQUIRE(invocation.getError() != std::string());
    }
}

TEST_CASE("Well-formed command-line is parsed correctly", "[Invocation]")
{
    Invocation invocation({ "uncover", ".", "show", "arg1", "arg2" });
    REQUIRE(invocation.getError() == std::string());
    CHECK(invocation.getRepositoryPath() == ".");
    CHECK(invocation.getSubcommandName() == "show");
    CHECK(invocation.getSubcommandArgs() == vs({ "arg1", "arg2" }));
}

TEST_CASE("Repository argument is optional", "[Invocation]")
{
    SECTION("No repo argument")
    {
        Invocation invocation({ "uncover", "show", "arg1", "arg2" });
        REQUIRE(invocation.getError() == std::string());
        CHECK(invocation.getRepositoryPath() == ".");
        CHECK(invocation.getSubcommandName() == "show");
        CHECK(invocation.getSubcommandArgs() == vs({ "arg1", "arg2" }));
    }

    SECTION("Path with slash")
    {
        Invocation invocation({ "uncover", "a/path", "show", "arg1", "arg2" });
        REQUIRE(invocation.getError() == std::string());
        CHECK(invocation.getRepositoryPath() == "a/path");
        CHECK(invocation.getSubcommandName() == "show");
        CHECK(invocation.getSubcommandArgs() == vs({ "arg1", "arg2" }));
    }

    SECTION("Subcommand without arguments")
    {
        Invocation invocation({ "uncover", "builds" });
        REQUIRE(invocation.getError() == std::string());
        CHECK(invocation.getRepositoryPath() == ".");
        CHECK(invocation.getSubcommandName() == "builds");
        CHECK(invocation.getSubcommandArgs() == vs({}));
    }
}
