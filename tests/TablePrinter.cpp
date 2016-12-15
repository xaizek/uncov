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

#include <sstream>
#include <stdexcept>
#include <string>

#include "TablePrinter.hpp"

TEST_CASE("Throws on wrong items", "[TablePrinter][format]")
{
    TablePrinter table({ "col" }, 80);

    REQUIRE_THROWS_AS(table.append({}), std::invalid_argument);
    REQUIRE_THROWS_AS(table.append({ "col1", "col2" }), std::invalid_argument);
}

TEST_CASE("Invisible columns turn into dots", "[TablePrinter][sizing]")
{
    TablePrinter table({ "title" }, 2);
    table.append({ "title" });

    std::ostringstream oss;
    table.print(oss);

    const std::string expected = "..\n"
                                 "..\n";
    REQUIRE(oss.str() == expected);
}

TEST_CASE("Longer column gets shortened first", "[TablePrinter][sizing]")
{
    TablePrinter table({ "id", "title" }, 8);
    table.append({ "id", "title" });

    std::ostringstream oss;
    table.print(oss);

    const std::string expected = "ID  T...\n"
                                 "id  t...\n";
    REQUIRE(oss.str() == expected);
}

TEST_CASE("Zero columns result in no output", "[TablePrinter][sizing]")
{
    TablePrinter table({}, 0);
    table.append({});

    std::ostringstream oss;
    table.print(oss);

    REQUIRE(oss.str() == std::string());
}

TEST_CASE("Zero terminal size results in no output", "[TablePrinter][sizing]")
{
    TablePrinter table({ "id", "title" }, 0);
    table.append({ "id", "title" });

    std::ostringstream oss;
    table.print(oss);

    REQUIRE(oss.str() == std::string());
}

TEST_CASE("Left and right alignment of columns", "[TablePrinter][alignment]")
{
    TablePrinter table({ "-id", "id" }, 80);
    table.append({ "a", "ccc" });
    table.append({ "bb", "bb" });
    table.append({ "ccc", "a" });

    std::ostringstream oss;
    table.print(oss);

    const std::string expected =
        "ID    ID\n"
        "a    ccc\n"
        "bb    bb\n"
        "ccc    a\n";
    REQUIRE(oss.str() == expected);
}
