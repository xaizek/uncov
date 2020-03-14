// Copyright (C) 2020 xaizek <xaizek@posteo.net>
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

#include "Settings.hpp"

static bool
operator==(const Settings &lhs, const Settings &rhs)
{
    return lhs.getTimeFormat() == rhs.getTimeFormat()
        && lhs.getMedLimit() == rhs.getMedLimit()
        && lhs.getHiLimit() == rhs.getHiLimit()
        && lhs.getTabSize() == rhs.getTabSize()
        && lhs.isColorOutputAllowed() == rhs.isColorOutputAllowed()
        && lhs.printLineNoInDiff() == rhs.printLineNoInDiff()
        && lhs.getMinFoldSize() == rhs.getMinFoldSize()
        && lhs.getFoldContext() == rhs.getFoldContext()
        && lhs.isHtmlOutput() == rhs.isHtmlOutput();
}

TEST_CASE("Loading from nonexistent file doesn't change anything", "[Settings]")
{
    Settings settings;
    settings.loadFromFile("no-such-file");
    CHECK(settings == Settings());
}

TEST_CASE("Loading from invalid file changes nothing", "[Settings]")
{
    Settings settings;
    settings.loadFromFile("tests/main.cpp");
    CHECK(settings == Settings());
}

TEST_CASE("Loading empty config changes nothing", "[Settings]")
{
    Settings settings;
    settings.loadFromFile("tests/test-configs/empty.ini");
    CHECK(settings == Settings());
}

TEST_CASE("Settings from correct config are applied", "[Settings]")
{
    Settings settings;
    CHECK(settings.getMedLimit() == 70.0f);
    CHECK(settings.getHiLimit() == 90.0f);
    CHECK(settings.getTabSize() == 4);
    CHECK(settings.getMinFoldSize() == 3);
    CHECK(settings.getFoldContext() == 1);
    CHECK(!settings.printLineNoInDiff());

    settings.loadFromFile("tests/test-configs/correct.ini");
    CHECK(settings.getMedLimit() == 50.5f);
    CHECK(settings.getHiLimit() == 75.0f);
    CHECK(settings.getTabSize() == 2);
    CHECK(settings.getMinFoldSize() == 4);
    CHECK(settings.getFoldContext() == 3);
    CHECK(settings.printLineNoInDiff());
}

TEST_CASE("Settings from incorrect config are ignored", "[Settings]")
{
    Settings settings;
    settings.loadFromFile("tests/test-configs/incorrect.ini");
    CHECK(settings == Settings());
}

TEST_CASE("Default values are representable in config", "[Settings]")
{
    Settings settings;
    settings.loadFromFile("tests/test-configs/default.ini");
    CHECK(settings == Settings());
}

TEST_CASE("Invalid values are handled sensibly", "[Settings]")
{
    SECTION("Inverted values are swapped")
    {
        Settings settings;
        settings.loadFromFile("tests/test-configs/inverted-bounds.ini");
        CHECK(settings.getMedLimit() == 50.5f);
        CHECK(settings.getHiLimit() == 75.0f);
    }
    SECTION("Out of range values are normalized")
    {
        Settings settings;
        settings.loadFromFile("tests/test-configs/out-of-range.ini");
        CHECK(settings.getMedLimit() == 0.0f);
        CHECK(settings.getHiLimit() == 100.0f);
        CHECK(settings.getTabSize() == 1);
        CHECK(settings.getMinFoldSize() == 1);
        CHECK(settings.getFoldContext() == 0);
    }
}
