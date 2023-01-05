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

#ifndef UNCOV_TESTS_TESTUTILS_HPP_
#define UNCOV_TESTS_TESTUTILS_HPP_

#include <iosfwd>
#include <sstream>
#include <string>
#include <vector>

#include "Settings.hpp"

class Repository;

class TestSettings : public Settings
{
public: // FilePrinterSettings only
    virtual bool isColorOutputAllowed() const override
    {
        return false;
    }
};

/**
 * @brief Temporarily redirects specified stream into a string.
 */
class StreamCapture
{
public:
    /**
     * @brief Constructs instance that redirects @p os.
     *
     * @param os Stream to redirect.
     */
    StreamCapture(std::ostream &os) : os(os)
    {
        rdbuf = os.rdbuf();
        os.rdbuf(oss.rdbuf());
    }

    /**
     * @brief Restores original state of the stream.
     */
    ~StreamCapture()
    {
        os.rdbuf(rdbuf);
    }

public:
    /**
     * @brief Retrieves captured output collected so far.
     *
     * @returns String containing the output.
     */
    std::string get() const
    {
        return oss.str();
    }

private:
    //! Stream that is being redirected.
    std::ostream &os;
    //! Temporary output buffer of the stream.
    std::ostringstream oss;
    //! Original output buffer of the stream.
    std::streambuf *rdbuf;
};

class TempDirCopy
{
public:
    TempDirCopy(const std::string &from, const std::string &to, bool force);

    TempDirCopy(const TempDirCopy &) = delete;
    TempDirCopy & operator=(const TempDirCopy &) = delete;

    ~TempDirCopy();

private:
    const std::string to;
};

class FileRestorer
{
public:
    FileRestorer(const std::string &from, const std::string &to);
    FileRestorer(const FileRestorer &) = delete;
    FileRestorer & operator=(const FileRestorer &) = delete;
    ~FileRestorer();

private:
    const std::string from;
    const std::string to;
};

/**
 * @brief Creates a @c std::vector<std::string> from initializer list.
 *
 * This is to be used in assertions, to shorten them and make more readable.
 *
 * @param v Temporary vector.
 *
 * @returns The vector.
 */
inline std::vector<std::string>
vs(std::vector<std::string> v)
{
    return v;
}

/**
 * @brief Creates a @c std::vector<int> from initializer list.
 *
 * This is to be used in assertions, to shorten them and make more readable.
 *
 * @param v Temporary vector.
 *
 * @returns The vector.
 */
inline std::vector<int>
vi(std::vector<int> v)
{
    return v;
}

/**
 * @brief Retrieves copy of default settings.
 *
 * @returns The settings.
 */
Settings & getSettings();

/**
 * @brief Retrieves path to a database file.
 *
 * @param repo Repository to operate on.
 *
 * @returns The path.
 */
std::string getDbPath(const Repository &repo);

/**
 * @brief Creates a compressed file in gzip format.
 *
 * @param path     Path of the file.
 * @param contents Contents of the file.
 */
void makeGz(const std::string &path, const std::string &contents);

/**
 * @brief Removes all occurances of a character @p c in a string @p str.
 *
 * @param str String for the operation.
 * @param c   Character to remove.
 */
void removeChars(std::string &str, char c);

#endif // UNCOV_TESTS_TESTUTILS_HPP_
