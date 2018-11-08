// Copyright (C) 2018 xaizek <xaizek@posteo.net>
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

#include <stdexcept>

#include "integration.hpp"

TEST_CASE("readProc handles empty command", "[integartion]")
{
    CHECK_THROWS_AS(readProc({ }, ".", CatchStderr{}), std::runtime_error);
}

TEST_CASE("readProc throws if program fails", "[integartion]")
{
    CHECK_THROWS_AS(readProc(std::vector<std::string>(10, "bad-command"), ".",
                             CatchStderr{}),
                    std::runtime_error);
}

TEST_CASE("queryProc fails on bad path", "[integartion]")
{
    CHECK(queryProc({ "echo" }, "no-such-dir") == EXIT_FAILURE);
}
