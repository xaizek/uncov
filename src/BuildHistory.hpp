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

#ifndef UNCOV_BUILDHISTORY_HPP_
#define UNCOV_BUILDHISTORY_HPP_

#include <boost/optional/optional_fwd.hpp>

#include <ctime>

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @file BuildHistory.hpp
 *
 * @brief This unit manages build history.
 */

class Build;
class BuildData;
class DB;
class File;

/**
 * @brief Interface used by Build class to load data lazily.
 */
class DataLoader
{
protected:
    //! Make destructor protected.
    ~DataLoader() = default;

public:
    /**
     * @brief Queries information about paths of a specific build.
     *
     * @param buildid Build ID.
     *
     * @returns Mappings of file paths to file IDs.
     */
    virtual std::map<std::string, int> loadPaths(int buildid) = 0;
    /**
     * @brief Loads file.
     *
     * @param fileid File ID.
     *
     * @returns File on success, empty optional otherwise.
     */
    virtual boost::optional<File> loadFile(int fileid) = 0;
};

/**
 * @brief Provides access to build history.
 */
class BuildHistory : private DataLoader
{
public:
    /**
     * @brief Creates an instance with the database.
     *
     * Updates database schema if necessary.
     *
     * @param db Database used as a storage.
     *
     * @throws std::runtime_error on database with too new schema.
     */
    explicit BuildHistory(DB &db);

public:
    /**
     * @brief Makes and stores new build in the database.
     *
     * @param buildData Data to construct the build from.
     *
     * @returns Just constructed build.
     */
    Build addBuild(const BuildData &buildData);

    /**
     * @brief Retrieves id of the last build.
     *
     * @returns The id or @c 0 if there are no builds.
     */
    int getLastBuildId();

    /**
     * @brief Retrieves id of the Nth last build.
     *
     * @param n Offset from the last build.
     *
     * @returns The id or @c 0 if there is no such build.
     */
    int getNToLastBuildId(int n);

    /**
     * @brief Retrieves id of the build previous to the given one.
     *
     * @param id Some existing build.
     *
     * @returns The id or @c 0 if there is no existing previous build.
     */
    int getPreviousBuildId(int id);

    /**
     * @brief Retrieves build by its ID.
     *
     * @param id Build ID to look for.
     *
     * @returns The build or an empty optional on wrong ID.
     */
    boost::optional<Build> getBuild(int id);

    /**
     * @brief Retrieves all builds.
     *
     * @returns The builds.
     */
    std::vector<Build> getBuilds();

    /**
     * @brief Retrieves all builds of the specified reference name.
     *
     * @param refName Name of the reference.
     *
     * @returns The builds.
     */
    std::vector<Build> getBuildsOn(const std::string &refName);

private:
    virtual std::map<std::string, int> loadPaths(int buildid) override;
    virtual boost::optional<File> loadFile(int fileid) override;

private:
    DB &db; //!< Reference to database, which stores build history.
};

/**
 * @brief Represents information about a single file.
 */
class File
{
public:
    /**
     * @brief Constructs file from its data.
     *
     * @param path     Path to the file.
     * @param hash     MD5 hash of contents of the file.
     * @param coverage Per-line coverage information.
     */
    File(std::string path, std::string hash, std::vector<int> coverage);

public:
    /**
     * @brief Retrieves path to the file within repository.
     *
     * @returns The path.
     */
    const std::string & getPath() const;
    /**
     * @brief Retrieves MD5 hash of contents of the file.
     *
     * @returns The hash.
     */
    const std::string & getHash() const;
    /**
     * @brief Retrieves per-line coverage information.
     *
     * Negative number indicates unrelevant line, otherwise it represents number
     * of hits for that line.
     *
     * @returns The coverage information.
     */
    const std::vector<int> & getCoverage() const;
    /**
     * @brief Retrieves number of covered lines.
     *
     * @returns The number.
     */
    int getCoveredCount() const;
    /**
     * @brief Retrieves number of lines that weren't covered.
     *
     * @returns The number.
     */
    int getMissedCount() const;

private:
    std::string path;          //!< Path to the file in repository.
    std::string hash;          //!< MD5 hash of the file.
    std::vector<int> coverage; //!< Per-line number of hits.
    int coveredCount;          //!< Number of covered lines.
    int missedCount;           //!< Number of missed lines.
};

/**
 * @brief Data that comprises build.
 *
 * This class exist to decouple Build class from constructing it.
 */
class BuildData
{
    /**
     * @brief Writes build data into the database.
     *
     * @param db Reference to the database, which acts as a storage.
     * @param bd Build data to write.
     *
     * @returns ID of build that was created.
     */
    friend int operator<<(DB &db, const BuildData &bd);

public:
    /**
     * @brief Constructs empty build.
     *
     * @param ref     Reference as an ID.
     * @param refName Same reference, but in symbolic form.
     */
    BuildData(std::string ref, std::string refName);

public:
    /**
     * @brief Adds file information to the build.
     *
     * @param file File data.
     */
    void addFile(File file);

private:
    const std::string ref;                       //!< Ref name as an ID.
    const std::string refName;                   //!< Symbolic ref name.
    std::unordered_map<std::string, File> files; //!< Files of the build.
};

/**
 * @brief Represents single build.
 */
class Build
{
public:
    /**
     * @brief Constructs a build.
     *
     * @param id           @copybrief id
     * @param ref          @copybrief ref
     * @param refName      @copybrief refName
     * @param coveredCount @copybrief coveredCount
     * @param missedCount  @copybrief missedCount
     * @param timestamp    @copybrief timestamp
     * @param loader       @copybrief loader
     */
    Build(int id, std::string ref, std::string refName,
          int coveredCount, int missedCount, int timestamp, DataLoader &loader);

public:
    /**
     * @brief Retrieves build ID.
     *
     * @returns The ID.
     */
    int getId() const;
    /**
     * @brief Retrieves reference as an ID.
     *
     * @returns The reference.
     */
    const std::string & getRef() const;
    /**
     * @brief Retrieves reference in symbolic form.
     *
     * @returns Symbolic reference.
     */
    const std::string & getRefName() const;
    /**
     * @brief Retrieves timestamp for this build.
     *
     * @returns The timestamp.
     */
    std::time_t getTimestamp() const;
    /**
     * @brief Retrieves total number of covered lines.
     *
     * @returns The number.
     */
    int getCoveredCount() const;
    /**
     * @brief Retrieves total number of lines that aren't covered.
     *
     * @returns The number.
     */
    int getMissedCount() const;
    /**
     * @brief Retrieves all paths that exist within this build.
     *
     * @returns The paths.
     */
    std::vector<std::string> getPaths() const;
    /**
     * @brief Retrieves file by its path.
     *
     * @param path Path to look up.
     *
     * @returns Object with information about the file or nothing on error.
     */
    boost::optional<File &> getFile(const std::string &path) const;

private:
    int id;                //!< Build ID.
    std::string ref;       //!< Ref name as an ID.
    std::string refName;   //!< Symbolic ref name.
    int coveredCount;      //!< Total number of covered lines.
    int missedCount;       //!< Total number of missed lines.
    std::time_t timestamp; //!< When the build was performed.
    DataLoader *loader;    //!< Reference to loader of file and path data.
    mutable std::map<std::string, int> pathMap;          //!< Cached paths.
    mutable std::unordered_map<std::string, File> files; //!< Cached files.
};

#endif // UNCOV_BUILDHISTORY_HPP_
