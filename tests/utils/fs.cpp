// Copyright (C) 2018 xaizek <xaizek@posteo.net>
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

#include <stdexcept>
#include <string>

#include "utils/fs.hpp"

TEST_CASE("readFile reads a file", "[utils-fs]")
{
    std::string contents = "int\n"
                           "main(int argc, char *argv[])\n"
                           "{\n"
                           "        return 0;\n"
                           "}\n";

    CHECK(readFile("tests/test-repo/test-file1.cpp") == contents);
}

TEST_CASE("readFile throws on a directory", "[utils-fs]")
{
    CHECK_THROWS_AS(readFile("tests"), const std::runtime_error &);
}

TEST_CASE("readFile throws on a nonexisting file", "[utils-fs]")
{
    CHECK_THROWS_AS(readFile("no-such-file"), const std::runtime_error &);
}
