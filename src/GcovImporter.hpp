// Copyright (C) 2018 xaizek <xaizek@posteo.net>
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

#ifndef UNCOV__GCOVIMPORTER_HPP__
#define UNCOV__GCOVIMPORTER_HPP__

#include <boost/filesystem/path.hpp>

#include <functional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

class File;

/**
 * @brief Generates information by calling `gcov` and collects it.
 */
class GcovImporter
{
    /**
     * @brief Accepts comand to be run at specified directory.
     */
    using runner_f = void(std::vector<std::string> &&cmd,
                          const std::string &dir);

public:
    /**
     * @brief Sets runner of external commands.
     *
     * There is no runner by default.
     *
     * @param runner New runner.
     */
    static void setRunner(std::function<runner_f> runner);

public:
    /**
     * @brief Does all the work.
     *
     * @param root Root of the source repository.
     * @param covoutRoot Root of subtree containing coverage data.
     * @param exclude List of paths to exclude.
     */
    GcovImporter(const std::string &root, const std::string &covoutRoot,
                 const std::vector<std::string> &exclude);

public:
    /**
     * @brief Retrieves coverage information.
     *
     * @returns Coverage information.
     */
    std::vector<File> && getFiles() &&;

private:
    /**
     * @brief Parses single `*.gcov` file.
     *
     * @param path Path of the file.
     */
    void parseGcov(const std::string &path);
    /**
     * @brief Checks whether specified path is excluded.
     *
     * @param path Path to check.
     *
     * @returns @c true if so, @c false otherwise.
     */
    bool isExcluded(boost::filesystem::path path) const;

private:
    //! Runner of external commands.
    static std::function<runner_f> runner;

private:
    //! Absolute and normalized path of the source repository.
    boost::filesystem::path rootDir;
    //! List of absolute and normalized path to be excluded.
    std::set<boost::filesystem::path> skipPaths;
    //! Temporary storage of coverage data during its collection.
    std::unordered_map<std::string, std::vector<int>> mapping;
    //! Final coverage information.
    std::vector<File> files;
};

#endif // UNCOV__GCOVIMPORTER_HPP__
