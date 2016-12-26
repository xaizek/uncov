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

TEST_CASE("Invalid input is detected", "[FileComparator]")
{
    const std::vector<std::string> file5 = { "a", "b", "c", "d", "e" };
    const std::vector<std::string> file6 = { "a", "b", "c", "d", "e", "f" };
    const std::vector<int> cov5 = { -1, -1, -1, -1, -1 };
    const std::vector<int> cov6 = { -1, -1, -1, -1, -1, -1 };

    SECTION("Invalid old information")
    {
        FileComparator comparator(file5, cov6, file6, cov6, false);
        REQUIRE(!comparator.isValidInput());
    }

    SECTION("Invalid new information")
    {
        FileComparator comparator(file6, cov6, file6, cov5, false);
        REQUIRE(!comparator.isValidInput());
    }

    SECTION("Invalid old and new information")
    {
        FileComparator comparator(file6, cov5, file5, cov6, false);
        REQUIRE(!comparator.isValidInput());
    }

    SECTION("Valid old and new information")
    {
        FileComparator comparator(file6, cov6, file5, cov5, false);
        REQUIRE(comparator.isValidInput());
    }
}

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

TEST_CASE("Files are compared by state", "[FileComparator]")
{
    const std::vector<std::string> file = { "a", "b", "c", "d", "e", "f" };
    const std::vector<int> covA = { -1, 10, -1, -1, -1, -1 };
    const std::vector<int> covB = { -1, 15, -1, -1, -1, -1 };

    FileComparator comparator(file, covA, file, covB, false);
    CHECK(comparator.areEqual());
    CHECK(comparator.getDiffSequence().size() == 1U);
}

TEST_CASE("Files are compared by hits", "[FileComparator]")
{
    const std::vector<std::string> file = { "a", "b", "c", "d", "e", "f" };
    const std::vector<int> covA = { -1, 10, -1, -1, -1, -1 };
    const std::vector<int> covB = { -1, 15, -1, -1, -1, -1 };

    FileComparator comparator(file, covA, file, covB, true);
    CHECK(!comparator.areEqual());

    const std::deque<DiffLine> &diff = comparator.getDiffSequence();
    REQUIRE(diff.size() == 6U);
    CHECK(diff[0].type == DiffLineType::Identical);
    CHECK(diff[1].type == DiffLineType::Common);
    CHECK(diff[2].type == DiffLineType::Identical);
    CHECK(diff[3].type == DiffLineType::Identical);
    CHECK(diff[4].type == DiffLineType::Identical);
    CHECK(diff[5].type == DiffLineType::Identical);
}

TEST_CASE("Files identical by state are detected", "[FileComparator]")
{
    const std::vector<std::string> file = { "a", "b", "c", "d", "e", "f" };
    const std::vector<int> covA = { -1, -1, -1, -1, -1, -1 };
    const std::vector<int> covB = { -1, -1, -1, -1, -1, -1 };

    FileComparator comparator(file, covA, file, covB, false);
    CHECK(comparator.areEqual());
    CHECK(comparator.getDiffSequence().size() == 1U);
}

TEST_CASE("Files identical by hits are detected", "[FileComparator]")
{
    const std::vector<std::string> file = { "a", "b", "c", "d", "e", "f" };
    const std::vector<int> covA = { -1, 10, -1, -1, -1, -1 };
    const std::vector<int> covB = { -1, 10, -1, -1, -1, -1 };

    FileComparator comparator(file, covA, file, covB, true);
    CHECK(comparator.areEqual());
    CHECK(comparator.getDiffSequence().size() == 1U);
}

TEST_CASE("Uninteresting changes are hidden", "[FileComparator]")
{
    SECTION("Modification")
    {
        const std::vector<std::string> fileA = { "a", "b", "c", "d", "e", "f" };
        const std::vector<std::string> fileB = { "x", "b", "c", "d", "e", "f" };
        const std::vector<int> cov = { -1, -1, -1, -1, -1, -1 };

        FileComparator comparator(fileA, cov, fileB, cov, false);
        CHECK(comparator.areEqual());
        CHECK(comparator.getDiffSequence().size() == 1U);
    }

    SECTION("Addition")
    {
        const std::vector<std::string> fileA = { "b", "c", "d", "e", "f" };
        const std::vector<std::string> fileB = { "x", "b", "c", "d", "e", "f" };
        const std::vector<int> covA = { -1, -1, -1, -1, -1 };
        const std::vector<int> covB = { -1, -1, -1, -1, -1, -1 };

        FileComparator comparator(fileA, covA, fileB, covB, false);
        CHECK(comparator.areEqual());
        CHECK(comparator.getDiffSequence().size() == 1U);
    }

    SECTION("Removal")
    {
        const std::vector<std::string> fileA = { "a", "b", "c", "d", "e", "f" };
        const std::vector<std::string> fileB = { "b", "c", "d", "e", "f" };
        const std::vector<int> covA = { -1, -1, -1, -1, -1, -1 };
        const std::vector<int> covB = { -1, -1, -1, -1, -1 };

        FileComparator comparator(fileA, covA, fileB, covB, false);
        CHECK(comparator.areEqual());
        CHECK(comparator.getDiffSequence().size() == 1U);
    }
}

TEST_CASE("Interesting changes are preserved", "[FileComparator]")
{
    const std::vector<std::string> fileA = { "a", "b", "c", "d", "e", "f" };
    const std::vector<std::string> fileB = { "x", "b", "c", "d", "e", "f" };
    const std::vector<int> covA = { 0, -1, -1, -1, -1, -1 };
    const std::vector<int> covB = { 20, -1, -1, -1, -1, -1 };

    FileComparator comparator(fileA, covA, fileB, covB, false);
    const std::deque<DiffLine> &diff = comparator.getDiffSequence();
    CHECK(!comparator.areEqual());
    REQUIRE(diff.size() >= 4U);
    CHECK(diff.size() == 4U);
    CHECK(diff[0].type == DiffLineType::Removed);
    CHECK(diff[1].type == DiffLineType::Added);
    CHECK(diff[2].type == DiffLineType::Identical);
    CHECK(diff[3].type == DiffLineType::Note);
}
