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
#include <map>

#include "DB.hpp"
#include "md5.hpp"

static std::string hashCoverage(const std::vector<int> &vec);
static void updateDBSchema(DB &db, int fromVersion);

/**
 * @brief Current database scheme version.
 */
const int AppDBVersion = 1;

File::File(std::string path, std::string hash, std::vector<int> coverage)
    : path(std::move(path)), hash(std::move(hash)),
      coverage(std::move(coverage))
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

const std::string &
File::getHash() const
{
    return hash;
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
        const std::string covHash = hashCoverage(coverage);

        int fileid = -1;

        for (std::tuple<int> val :
            db.queryAll("SELECT fileid FROM files "
                        "WHERE path = :path AND hash = :hash AND "
                              "covhash = :covhash",
                        { ":path"_b = file.getPath(),
                          ":hash"_b = file.getHash(),
                          ":covhash"_b = covHash })) {
            fileid = std::get<0>(val);
        }

        if (fileid == -1) {
            db.execute("INSERT INTO files (path, hash, covhash, coverage) "
                       "VALUES (:path, :hash, :covhash, :coverage)",
                       { ":path"_b = file.getPath(),
                         ":hash"_b = file.getHash(),
                         ":covhash"_b = covHash,
                         ":coverage"_b = coverage });
            fileid = db.getLastRowId();
        }

        db.execute("INSERT INTO filemap (buildid, fileid) "
                   "VALUES (:buildid, :fileid)",
                   { ":buildid"_b = buildid,
                     ":fileid"_b = fileid });
    }

    transaction.commit();

    return buildid;
}

/**
 * @brief Hashes coverage vector into a string.
 *
 * @param vec Coverage to hash.
 *
 * @returns String containing MD5 hash of the coverage.
 */
static std::string
hashCoverage(const std::vector<int> &vec)
{
    std::ostringstream oss;
    std::copy(vec.cbegin(), vec.cend(), std::ostream_iterator<int>(oss, " "));
    return md5(oss.str());
}

Build::Build(int id, std::string ref, std::string refName,
             int coveredCount, int uncoveredCount, DataLoader &loader)
    : id(id), ref(std::move(ref)), refName(std::move(refName)),
      coveredCount(coveredCount), uncoveredCount(uncoveredCount),
      loader(&loader)
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

std::vector<std::string>
Build::getPaths() const
{
    // Make sure file path to file id mapping is loaded.
    if (pathMap.empty()) {
        pathMap = loader->loadPaths(id);
    }

    std::vector<std::string> paths;
    paths.reserve(pathMap.size());
    for (const auto &entry : pathMap) {
        paths.push_back(entry.first);
    }
    return paths;
}

boost::optional<File &>
Build::getFile(const std::string &path) const
{
    // Check if this file was already loaded.
    const auto fileMatch = files.find(path);
    if (fileMatch != files.end()) {
        return fileMatch->second;
    }

    // Make sure file path to file id mapping is loaded.
    if (pathMap.empty()) {
        pathMap = loader->loadPaths(id);
    }

    // Requested file should be in the map.
    const auto pathMatch = pathMap.find(path);
    if (pathMatch == pathMap.end()) {
        return {};
    }

    // Load the file and cache it.
    if (boost::optional<File> file = loader->loadFile(pathMatch->second,
                                                      path)) {
        return files.emplace(path, std::move(*file)).first->second;
    }

    return {};
}

BuildHistory::BuildHistory(DB &db) : db(db)
{
    std::tuple<int> vals = db.queryOne("pragma user_version");

    const int fileDBVersion = std::get<0>(vals);
    if (fileDBVersion > AppDBVersion) {
        throw std::runtime_error("Database schema version is newer than "
                                 "supported by the application (up to " +
                                 std::to_string(AppDBVersion) + "): " +
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
                CREATE TABLE builds (
                    buildid INTEGER,
                    vcsref TEXT NOT NULL,
                    vcsrefname TEXT NOT NULL,
                    covered INTEGER,
                    uncovered INTEGER,

                    PRIMARY KEY (buildid)
                )
            )");
            db.execute(R"(
                CREATE TABLE files (
                    fileid INTEGER,
                    path TEXT NOT NULL,
                    hash TEXT NOT NULL,
                    covhash TEXT NOT NULL,
                    coverage BLOB NOT NULL,

                    PRIMARY KEY (fileid)
                )
            )");
            db.execute(R"(
                CREATE TABLE filemap (
                    buildid INTEGER,
                    fileid INTEGER,

                    FOREIGN KEY (buildid) REFERENCES builds(buildid),
                    FOREIGN KEY (fileid) REFERENCES files(fileid)
                )
            )");
            // Fall through.
        case 1:
            break;
    }

    db.execute("pragma user_version = " + std::to_string(AppDBVersion));
    transaction.commit();

    // Compact database after migration by defragmenting it.
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

int
BuildHistory::getPreviousBuildId(int id)
{
    // TODO: try looking for closest build in terms of commits.
    return id - 1;
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

std::map<std::string, int>
BuildHistory::loadPaths(int buildid)
{
    std::map<std::string, int> paths;
    for (std::tuple<std::string, int> vals : db.queryAll(
            "SELECT path, fileid FROM files NATURAL JOIN filemap "
            "WHERE buildid = :buildid",
            { ":buildid"_b = buildid })) {
        paths.emplace(std::move(std::get<0>(vals)), std::get<1>(vals));
    }
    return paths;
}

boost::optional<File>
BuildHistory::loadFile(int fileid, const std::string &path)
{
    try {
        std::tuple<std::string, std::vector<int>> vals =
            db.queryOne("SELECT hash, coverage FROM files "
                        "WHERE fileid = :fileid",
                        { ":fileid"_b = fileid });

        return File(path, std::move(std::get<0>(vals)),
                    std::move(std::get<1>(vals)));
    } catch (const std::runtime_error &) {
        return {};
    }
}
