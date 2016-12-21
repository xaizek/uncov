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

#include "Repository.hpp"

TEST_CASE("Repository is discovered by nested path", "[Repository]")
{
    REQUIRE_NOTHROW(Repository repo("tests/test-repo/subdir"));
}

TEST_CASE("Repository throws on wrong path", "[Repository]")
{
    REQUIRE_THROWS_AS(Repository repo("/no-such-path"), std::invalid_argument);
}
