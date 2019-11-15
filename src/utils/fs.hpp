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

#ifndef UNCOV__UTILS__FS_HPP__
#define UNCOV__UTILS__FS_HPP__

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include <string>

/**
 * @file fs.hpp
 *
 * @brief File-system utilities.
 */

/**
 * @brief Temporary directory in RAII-style.
 */
class TempDir
{
public:
    /**
     * @brief Makes temporary directory, which is removed in destructor.
     *
     * @param prefix Directory name prefix.
     */
    explicit TempDir(const std::string &prefix)
    {
        namespace fs = boost::filesystem;

        path = fs::temp_directory_path()
             / fs::unique_path("uncov-" + prefix + "-%%%%-%%%%");
        fs::create_directories(path);
    }

    // Make sure temporary directory is deleted only once.
    TempDir(const TempDir &rhs) = delete;
    TempDir & operator=(const TempDir &rhs) = delete;

    /**
     * @brief Removes temporary directory and all its content, if it still
     *        exists.
     */
    ~TempDir()
    {
        boost::filesystem::remove_all(path);
    }

public:
    /**
     * @brief Provides implicit conversion to a directory path string.
     *
     * @returns The path.
     */
    operator std::string() const
    {
        return path.string();
    }

private:
    /**
     * @brief Path to the temporary directory.
     */
    boost::filesystem::path path;
};

/**
 * @brief Checks that @p path is somewhere under @p root.
 *
 * @param root Root to check against.
 * @param path Path to check.
 *
 * @returns @c true if so, otherwise @c false.
 *
 * @note Path are assumed to be canonicalized.
 */
bool pathIsInSubtree(const boost::filesystem::path &root,
                     const boost::filesystem::path &path);

/**
 * @brief Excludes `..` and `.` entries from a path.
 *
 * @param path Path to process.
 *
 * @returns Normalized path.
 */
boost::filesystem::path normalizePath(const boost::filesystem::path &path);

/**
 * @brief Makes path relative to specified base directory.
 *
 * @param base Base path.
 * @param path Path to make relative.
 *
 * @returns Relative path.
 */
boost::filesystem::path makeRelativePath(boost::filesystem::path base,
                                         boost::filesystem::path path);

/**
 * @brief Reads file into a string.
 *
 * @param path Path to the file.
 *
 * @returns File contents.
 *
 * @throws std::runtime_error if @p path specifies a directory or file
 *                            reading has failed.
 */
std::string readFile(const std::string &path);

#endif // UNCOV__UTILS__FS_HPP__
