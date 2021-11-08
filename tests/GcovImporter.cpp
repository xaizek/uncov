// Copyright (C) 2021 xaizek <xaizek@posteo.net>
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

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/scope_exit.hpp>

#include <fstream>
#include <stdexcept>
#include <string>

#include "utils/fs.hpp"
#include "GcovImporter.hpp"

#include "TestUtils.hpp"

namespace fs = boost::filesystem;

TEST_CASE("Need support for at least one intermediate format", "[GcovImporter]")
{
    GcovInfo gcovInfo(/*employBinning=*/false, /*jsonFormat=*/false,
                      /*intermediateFormat=*/false);
    REQUIRE_THROWS_AS(GcovImporter("", "", {}, "", gcovInfo),
                      const std::runtime_error &);
}

TEST_CASE("Plain text format parsed and binning is performed", "[GcovImporter]")
{
    TempDir tempDir("gcovimporter");
    std::string tempDirPath = tempDir;

    fs::create_directory(tempDirPath + "/src");
    fs::create_directory(tempDirPath + "/tests");

    std::ofstream{tempDirPath + "/src/file.gcno"} << "\n";
    std::ofstream{tempDirPath + "/tests/file.gcno"} << "\n";

    auto runner = [](std::vector<std::string> &&cmd, const std::string &dir) {
        CHECK(cmd.size() == 5);

        const fs::path inPath = cmd.back();
        const fs::path relPath = inPath.parent_path().filename().string()
                               / inPath.filename();
        const std::string outName = inPath.parent_path().filename().string()
                                  + inPath.filename().string();
        const std::string outPath = dir + '/' + outName + ".gcov";

        std::ofstream{outPath}
            << "file:" << relPath.string() << '\n'
            << "lcount:1,1\n"
            << "lcount:2,0\n";
    };
    auto prevRunner = GcovImporter::setRunner(runner);
    BOOST_SCOPE_EXIT_ALL(prevRunner) {
        GcovImporter::setRunner(prevRunner);
    };

    GcovInfo gcovInfo(/*employBinning=*/true, /*jsonFormat=*/false,
                      /*intermediateFormat=*/true);
    std::vector<File> files =
        GcovImporter(tempDirPath, tempDirPath, {}, tempDirPath,
                     gcovInfo).getFiles();

    REQUIRE(files.size() == 2);
    CHECK(files[0].getCoveredCount() == 1);
    CHECK(files[0].getMissedCount() == 1);
    CHECK(files[1].getCoveredCount() == 1);
    CHECK(files[1].getMissedCount() == 1);
}

TEST_CASE("JSON format is parsed", "[GcovImporter]")
{
    TempDir tempDir("gcovimporter");
    std::string tempDirPath = tempDir;

    std::ofstream{tempDirPath + "/file.gcno"} << "\n";

    auto runner = [](std::vector<std::string> &&/*cmd*/,
                     const std::string &dir) {
        makeGz(dir + "/out.gcov.json.gz", R"({
            "files": [
                {
                    "file": "file.gcno",
                    "lines": [
                        { "line_number": 1, "count": 1 },
                        { "line_number": 2, "count": 0 }
                    ]
                },
                {
                    "file": "/usr/include/whatever.h",
                    "lines": [ { "line_number": 1, "count": 1 } ]
                }
            ]
        })");
    };
    auto prevRunner = GcovImporter::setRunner(runner);
    BOOST_SCOPE_EXIT_ALL(prevRunner) {
        GcovImporter::setRunner(prevRunner);
    };

    GcovInfo gcovInfo(/*employBinning=*/false, /*jsonFormat=*/true,
                      /*intermediateFormat=*/true);
    std::vector<File> files =
        GcovImporter(tempDirPath, tempDirPath, {}, tempDirPath,
                     gcovInfo).getFiles();

    REQUIRE(files.size() == 1);
    CHECK(files[0].getCoveredCount() == 1);
    CHECK(files[0].getMissedCount() == 1);
}
