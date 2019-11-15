// Copyright (C) 2016 xaizek <xaizek@posteo.net>
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

#include <boost/optional.hpp>

#include "BuildHistory.hpp"
#include "DB.hpp"
#include "Repository.hpp"

#include "TestUtils.hpp"

TEST_CASE("BuildHistory throws on too new database schema", "[BuildHistory]")
{
    Repository repo("tests/test-repo/subdir");
    const std::string dbPath = repo.getGitPath() + "/uncov.sqlite";
    FileRestorer databaseRestorer(dbPath, dbPath + "_original");
    DB db(dbPath);
    db.execute("pragma user_version = 99999");

    REQUIRE_THROWS_AS(BuildHistory bh(db), std::runtime_error);
}

TEST_CASE("List of builds on unknown branch is empty", "[BuildHistory]")
{
    Repository repo("tests/test-repo/subdir");
    const std::string dbPath = repo.getGitPath() + "/uncov.sqlite";
    FileRestorer databaseRestorer(dbPath, dbPath + "_original");
    DB db(dbPath);

    REQUIRE(BuildHistory(db).getBuildsOn(":wrong").empty());
}

TEST_CASE("File is loaded from database", "[Build][File]")
{
    Repository repo("tests/test-repo/subdir");
    DB db(repo.getGitPath() + "/uncov.sqlite");
    BuildHistory bh(db);

    boost::optional<Build> build = bh.getBuild(1);
    REQUIRE(build);

    /* File with too few lines wasn't loaded due to coverage decompression
     * error, so just see if it's loaded and contains valid data. */
    boost::optional<File &> file = build->getFile("test-file1.cpp");
    REQUIRE(file);

    REQUIRE(file->getCoverage() == vi({ -1, 1, -1, 1, -1 }));
}
