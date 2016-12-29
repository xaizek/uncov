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

#ifndef UNCOV__TESTS__TESTUTILS_HPP__
#define UNCOV__TESTS__TESTUTILS_HPP__

#include <iosfwd>
#include <sstream>
#include <string>
#include <vector>

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
    /**
     * @brief Stream that is being redirected.
     */
    std::ostream &os;
    /**
     * @brief Temporary output buffer of the stream.
     */
    std::ostringstream oss;
    /**
     * @brief Original output buffer of the stream.
     */
    std::streambuf *rdbuf;
};

class TempDirCopy
{
public:
    TempDirCopy(const std::string &from, const std::string &to);

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

#endif // UNCOV__TESTS__TESTUTILS_HPP__
