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

#include "BuildHistory.hpp"

#include <boost/optional.hpp>

#include <algorithm>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "DB.hpp"

static void updateDBSchema(DB &db, int fromVersion);

/**
 * @brief Current database scheme version.
 */
const int AppDBVersion = 2;

File::File(std::string path, std::vector<int> coverage)
    : path(std::move(path)), coverage(std::move(coverage))
{
    coveredCount = 0;
    uncoveredCount = 0;
    for (int hits : this->coverage) {
        if (hits == 0) {
            ++uncoveredCount;
        } else if (hits > 0) {
            ++coveredCount;
        }
    }
}

const std::string &
File::getPath() const
{
    return path;
}

const std::vector<int> &
File::getCoverage() const
{
    return coverage;
}

int
File::getCoveredCount() const
{
    return coveredCount;
}

int
File::getUncoveredCount() const
{
    return uncoveredCount;
}

BuildData::BuildData(std::string ref, std::string refName)
    : ref(std::move(ref)), refName(std::move(refName))
{
}

const std::string &
BuildData::getRef() const
{
    return ref;
}

const std::string &
BuildData::getRefName() const
{
    return refName;
}

void
BuildData::addFile(File file)
{
    files.emplace(file.getPath(), std::move(file));
}

int
operator<<(DB &db, const BuildData &bd)
{
    int coveredCount = 0;
    int uncoveredCount = 0;
    for (auto entry : bd.files) {
        File &file = entry.second;
        coveredCount += file.getCoveredCount();
        uncoveredCount += file.getUncoveredCount();
    }

    Transaction transaction = db.makeTransaction();

    db.execute("INSERT INTO builds (vcsref, vcsrefname, covered, uncovered) "
               "VALUES (:ref, :refname, :covered, :uncovered)",
               { ":ref"_b = bd.getRef(),
                 ":refname"_b = bd.getRefName(),
                 ":covered"_b = coveredCount,
                 ":uncovered"_b = uncoveredCount });

    const int buildid = db.getLastRowId();

    for (auto entry : bd.files) {
        File &file = entry.second;

        const std::vector<int> &coverage = file.getCoverage();

        db.execute("INSERT INTO files (buildid, path, coverage) "
                   "VALUES (:buildid, :path, :coverage)",
                   { ":buildid"_b = buildid,
                     ":path"_b = file.getPath(),
                     ":coverage"_b = coverage });
    }

    transaction.commit();

    return buildid;
}

Build::Build(int id, std::string ref, std::string refName,
             int coveredCount, int uncoveredCount, DataLoader &loader)
    : id(id), ref(std::move(ref)), refName(std::move(refName)),
      coveredCount(coveredCount), uncoveredCount(uncoveredCount), loader(loader)
{
}

int
Build::getId() const
{
    return id;
}

const std::string &
Build::getRef() const
{
    return ref;
}

const std::string &
Build::getRefName() const
{
    return refName;
}

int
Build::getCoveredCount() const
{
    return coveredCount;
}

int
Build::getUncoveredCount() const
{
    return uncoveredCount;
}

const std::vector<std::string> &
Build::getPaths() const
{
    if (paths.empty()) {
        paths = loader.loadPaths(id);
    }
    return paths;
}

boost::optional<File &>
Build::getFile(const std::string &path) const
{
    auto match = files.find(path);
    if (match != files.end()) {
        return match->second;
    }

    if (boost::optional<File> file = loader.loadFile(id, path)) {
        return files.emplace(path, *file).first->second;
    }
    return {};
}

BuildHistory::BuildHistory(DB &db) : db(db)
{
    std::tuple<int> vals = db.queryOne("pragma user_version");

    const int fileDBVersion = std::get<0>(vals);
    if (fileDBVersion > AppDBVersion) {
        throw std::runtime_error("Database is from newer version: " +
                                 std::to_string(fileDBVersion));
    }

    if (fileDBVersion < AppDBVersion) {
        updateDBSchema(db, fileDBVersion);
    }
}

/**
 * @brief Performs update of database scheme to the latest version.
 *
 * This process might take some time.  Should either succeed or be no-op.
 *
 * @param db Database to update.
 * @param fromVersion Current version of scheme.
 */
static void
updateDBSchema(DB &db, int fromVersion)
{
    Transaction transaction = db.makeTransaction();

    switch (fromVersion) {
        case 0:
            db.execute(R"(
                CREATE TABLE IF NOT EXISTS builds (
                    buildid INTEGER,
                    vcsref TEXT NOT NULL,
                    vcsrefname TEXT NOT NULL,
                    covered INTEGER,
                    uncovered INTEGER,

                    PRIMARY KEY (buildid)
                )
            )");

            db.execute(R"(
                CREATE TABLE IF NOT EXISTS files (
                    buildid INTEGER,
                    path TEXT NOT NULL,
                    coverage TEXT NOT NULL,

                    PRIMARY KEY (buildid, path),
                    FOREIGN KEY (buildid) REFERENCES builds(buildid)
                )
            )");
            // Fall through.
        case 1:
            db.execute(R"(
                CREATE TABLE files_new (
                    buildid INTEGER,
                    path TEXT NOT NULL,
                    coverage BLOB NOT NULL,

                    PRIMARY KEY (buildid, path),
                    FOREIGN KEY (buildid) REFERENCES builds(buildid)
                )
            )");
            for (std::tuple<int, std::string, std::string> vals :
                db.queryAll("SELECT buildid, path, coverage FROM files")) {
                std::istringstream is(std::get<2>(vals));
                std::vector<int> coverage;
                for (int i; is >> i; ) {
                    coverage.push_back(i);
                }

                db.execute("INSERT INTO files_new (buildid, path, coverage) "
                           "VALUES (:buildid, :path, :coverage)",
                           { ":buildid"_b = std::get<0>(vals),
                             ":path"_b = std::get<1>(vals),
                             ":coverage"_b = coverage });
            }
            db.execute("DROP TABLE files");
            db.execute("ALTER TABLE files_new RENAME TO files");
            break;
    }

    db.execute("pragma user_version = " + std::to_string(AppDBVersion));
    transaction.commit();
    db.execute("VACUUM");
}

Build
BuildHistory::addBuild(const BuildData &buildData)
{
    const int buildid = (db << buildData);
    return *getBuild(buildid);
}

int
BuildHistory::getLastBuildId()
{
    try {
        std::tuple<int> vals = db.queryOne("SELECT buildid FROM builds "
                                           "ORDER BY buildid DESC LIMIT 1");
        return std::get<0>(vals);
    } catch (const std::runtime_error &) {
        return 0;
    }
}

boost::optional<Build>
BuildHistory::getBuild(int id)
{
    try {
        DataLoader &loader = *this;
        std::tuple<std::string, std::string, int, int> vals =
            db.queryOne("SELECT vcsref, vcsrefname, covered, uncovered "
                        "FROM builds WHERE buildid = :buildid",
                        { ":buildid"_b = id } );
        return Build(id, std::get<0>(vals), std::get<1>(vals),
                     std::get<2>(vals), std::get<3>(vals), loader);
    } catch (const std::runtime_error &) {
        return {};
    }
}

std::vector<Build>
BuildHistory::getBuilds()
{
    std::vector<Build> builds;
    DataLoader &loader = *this;
    for (std::tuple<int, std::string, std::string, int, int> vals :
         db.queryAll("SELECT buildid, vcsref, vcsrefname, covered, uncovered "
                     "FROM builds")) {
        builds.emplace_back(std::get<0>(vals), std::get<1>(vals),
                            std::get<2>(vals), std::get<3>(vals),
                            std::get<4>(vals), loader);
    }
    return builds;
}

std::vector<std::string>
BuildHistory::loadPaths(int id)
{
    std::vector<std::string> paths;
    for (std::tuple<std::string> vals : db.queryAll("SELECT path FROM files "
                                                    "WHERE buildid = :buildid",
                                                    { ":buildid"_b = id })) {
        paths.push_back(std::get<0>(vals));
    }
    return paths;
}

boost::optional<File>
BuildHistory::loadFile(int id, const std::string &path)
{
    try {
        std::tuple<std::vector<int>> vals =
            db.queryOne("SELECT coverage FROM files "
                        "WHERE buildid = :buildid AND path = :path",
                        { ":buildid"_b = id, ":path"_b = path });

        return File(path, std::move(std::get<0>(vals)));
    } catch (const std::runtime_error &) {
        return {};
    }
}
