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

#include <boost/optional.hpp>

#include "BuildHistory.hpp"
#include "DB.hpp"
#include "Repository.hpp"

#include "TestUtils.hpp"

TEST_CASE("File is loaded from database", "[Build][File]")
{
    Repository repo("tests/test-repo/subdir");
    DB db(repo.getGitPath() + "/uncover.sqlite");
    BuildHistory bh(db);

    boost::optional<Build> build = bh.getBuild(1);
    REQUIRE(build);

    /* File with too few lines wasn't loaded due to coverage decompression
     * error, so just see if it's loaded and contains valid data. */
    boost::optional<File &> file = build->getFile("test-file1.cpp");
    REQUIRE(file);

    REQUIRE(file->getCoverage() == vi({ -1, 1, -1, 1, -1 }));
}