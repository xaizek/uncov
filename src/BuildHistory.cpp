#include "BuildHistory.hpp"

#include <boost/optional.hpp>

#include <algorithm>
#include <iterator>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "DB.hpp"

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

    std::ostringstream oss;
    for (auto entry : bd.files) {
        File &file = entry.second;

        oss.str(std::string());
        const std::vector<int> &coverage = file.getCoverage();
        std::copy(coverage.cbegin(), coverage.cend(),
                  std::ostream_iterator<int>(oss, " "));

        db.execute("INSERT INTO files (buildid, path, coverage) "
                   "VALUES (:buildid, :path, :coverage)",
                   { ":buildid"_b = buildid,
                     ":path"_b = file.getPath(),
                     ":coverage"_b = oss.str() });
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
}

Build
BuildHistory::addBuild(const BuildData &buildData)
{
    const int buildid = (db << buildData);
    return *getBuild(buildid);
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
        std::tuple<std::string> vals =
            db.queryOne("SELECT coverage FROM files "
                        "WHERE buildid = :buildid AND path = :path",
                        { ":buildid"_b = id, ":path"_b = path });

        std::istringstream is(std::get<0>(vals));

        std::vector<int> coverage;
        for (int i; is >> i; ) {
            coverage.push_back(i);
        }

        return File(path, std::move(coverage));
    } catch (const std::runtime_error &) {
        return {};
    }
}
