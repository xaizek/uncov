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

#include <stdexcept>

#include "utils/strings.hpp"

#include "TestUtils.hpp"

TEST_CASE("splitAt throws on absent delimiter", "[utils-strings]")
{
    CHECK_THROWS_AS(splitAt("a b", ','), std::runtime_error);
}

TEST_CASE("split() handles empty string", "[utils-strings]")
{
    CHECK(split("", ':') == vs({}));
}
