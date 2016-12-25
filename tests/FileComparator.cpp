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

#include <string>
#include <vector>

#include "FileComparator.hpp"

TEST_CASE("Context line at beginning of file is folded", "[FileComparator]")
{
    const std::vector<std::string> file = { "a", "b", "c", "d", "e", "f" };
    const std::vector<int> covA = { -1, -1, -1, -1, -1, -1 };
    const std::vector<int> covB = { -1, -1, -1, -1, -1, 0 };

    FileComparator comparator(file, covA, file, covB, false);

    const std::deque<DiffLine> &diff = comparator.getDiffSequence();
    REQUIRE(diff.size() == 3U);
    CHECK(diff[0].type == DiffLineType::Note);
    CHECK(diff[1].type == DiffLineType::Identical);
    CHECK(diff[2].type == DiffLineType::Common);
}

TEST_CASE("Context line at end of file is folded", "[FileComparator]")
{
    const std::vector<std::string> file = { "a", "b", "c", "d", "e", "f" };
    const std::vector<int> covA = { -1, -1, -1, -1, -1, -1 };
    const std::vector<int> covB = { 0, -1, -1, -1, -1, -1 };

    FileComparator comparator(file, covA, file, covB, false);

    const std::deque<DiffLine> &diff = comparator.getDiffSequence();
    REQUIRE(diff.size() == 3U);
    CHECK(diff[0].type == DiffLineType::Common);
    CHECK(diff[1].type == DiffLineType::Identical);
    CHECK(diff[2].type == DiffLineType::Note);
}
