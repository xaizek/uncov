// Copyright (C) 2017 xaizek <xaizek@posteo.net>
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

#include <sstream>
#include <string>

#include "utils/Text.hpp"
#include "FileComparator.hpp"
#include "FilePrinter.hpp"
#include "Settings.hpp"

#include "TestUtils.hpp"

TEST_CASE("Annotated file contents is printed", "[FilePrinter]")
{
    std::ostringstream oss;

    FilePrinter printer(*getSettings());
    printer.print(oss, "path", "line1\nline2\nline3\n", { -1, 0, 1 });

    const std::string expected = "    1       : line1\n"
                                 "    2    x0 : line2\n"
                                 "    3    x1 : line3\n";
    REQUIRE(oss.str() == expected);
}

TEST_CASE("File contents length and coverage length is checked",
          "[FilePrinter]")
{
    std::string contents;
    std::string expected;

    SECTION("Too few file lines")
    {
        contents = "line1\n";
        expected = "    1       : line1\n";
    }

    SECTION("Too many file lines")
    {
        contents = "line1\nline2\nline3\n";
        expected = "    1       : line1\n"
                   "    2    x0 : line2\n"
                   "    3 ERROR : line3\n";
    }

    std::ostringstream oss;
    FilePrinter printer(*getSettings());
    printer.print(oss, "path", contents, { -1, 0 });

    REQUIRE(boost::starts_with(oss.str(), expected + "ERROR: "));
}

TEST_CASE("File diffing works", "[FilePrinter]")
{
    Text oldVersion("line1\nline2\nline3\nline4\nline5\nline6\nline7\n");
    Text newVersion("line0\nline2\nline3\nline4\nline5\nline6\nline7\n");
    std::vector<int> oldCov = { 10, 5, -1, -1, -1, -1, -1 };
    std::vector<int> newCov = { 11, 10, -1, -1, -1, -1, -1 };

    FileComparator comparator(oldVersion.asLines(), oldCov,
                              newVersion.asLines(), newCov,
                              true, *getSettings());

    std::ostringstream oss;
    FilePrinter printer(*getSettings());
    printer.printDiff(oss, "path",
                      oldVersion.asStream(), oldCov,
                      newVersion.asStream(), newCov,
                      comparator);

    const std::string expected = "  x10 :      :-line1\n"
                                 "      :  x11 :+line0\n"
                                 "   x5 :  x10 : line2\n"
                                 "      :      : line3\n"
                                 " <<< 4 lines folded >>>\n";

    REQUIRE(oss.str() == expected);
}

TEST_CASE("Can leave only missed lines", "[FilePrinter]")
{
    std::ostringstream oss;

    FilePrinter printer(*getSettings());
    printer.print(oss, "path",
                  "line1\nline2\nline3\nline4\nline5\nline6\nline7\n",
                  { 0, 0, -1, -1, -1, -1, -1 }, true);

    const std::string expected = "    1    x0 : line1\n"
                                 "    2    x0 : line2\n"
                                 "    3       : line3\n"
                                 " <<< 4 lines folded >>>\n";
    REQUIRE(oss.str() == expected);
}
