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

#ifndef UNCOV__BUILDHISTORY_HPP__
#define UNCOV__BUILDHISTORY_HPP__

#include <boost/optional/optional_fwd.hpp>

#include <ctime>

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

class Build;
class BuildData;
class DB;
class File;

class DataLoader
{
protected:
    ~DataLoader() = default;

public:
    virtual std::map<std::string, int> loadPaths(int buildid) = 0;
    virtual boost::optional<File> loadFile(int fileid) = 0;
};

class BuildHistory : private DataLoader
{
public:
    /**
     * @brief Creates an instance with the database.
     *
     * @param db Database used as a storage.
     *
     * @throws std::runtime_error on database with too new schema.
     */
    BuildHistory(DB &db);

public:
    Build addBuild(const BuildData &buildData);

    /**
     * @brief Retrieves id of the last build.
     *
     * @returns The id or @c 0 if there is no builds.
     */
    int getLastBuildId();

    /**
     * @brief Retrieves id of the build previous to the given one.
     *
     * @param id Some existing build.
     *
     * @returns The id or @c 0 if there is no existing previous build.
     */
    int getPreviousBuildId(int id);

    boost::optional<Build> getBuild(int id);
    std::vector<Build> getBuilds();

private:
    virtual std::map<std::string, int> loadPaths(int buildid) override;
    virtual boost::optional<File> loadFile(int fileid) override;

private:
    DB &db;
};

class File
{
public:
    File(std::string path, std::string hash, std::vector<int> coverage);

public:
    const std::string & getPath() const;
    const std::string & getHash() const;
    const std::vector<int> & getCoverage() const;
    int getCoveredCount() const;
    int getMissedCount() const;

private:
    const std::string path;
    const std::string hash;
    const std::vector<int> coverage;
    int coveredCount;
    int missedCount;
};

class BuildData
{
    friend int operator<<(DB &db, const BuildData &bd);

public:
    BuildData(std::string ref, std::string refName);

public:
    const std::string & getRef() const;
    const std::string & getRefName() const;
    void addFile(File file);

private:
    const std::string ref;
    const std::string refName;
    std::unordered_map<std::string, File> files;
};

class Build
{
public:
    Build(int id, std::string ref, std::string refName,
          int coveredCount, int missedCount, int timestamp, DataLoader &loader);

public:
    int getId() const;
    const std::string & getRef() const;
    const std::string & getRefName() const;
    std::time_t getTimestamp() const;
    int getCoveredCount() const;
    int getMissedCount() const;
    std::vector<std::string> getPaths() const;
    boost::optional<File &> getFile(const std::string &path) const;

private:
    int id;
    std::string ref;
    std::string refName;
    int coveredCount;
    int missedCount;
    std::time_t timestamp;
    DataLoader *loader;
    mutable std::map<std::string, int> pathMap;
    mutable std::unordered_map<std::string, File> files;
};

#endif // UNCOV__BUILDHISTORY_HPP__
