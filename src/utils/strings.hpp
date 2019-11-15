// Copyright (C) 2017 xaizek <xaizek@posteo.net>
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

#ifndef UNCOV__UTILS__STRINGS_HPP__
#define UNCOV__UTILS__STRINGS_HPP__

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

/**
 * @brief Splits string in two parts at the leftmost delimiter.
 *
 * @param s String to split.
 * @param delim Delimiter, which separates left and right parts of the string.
 *
 * @returns Pair of left and right string parts.
 *
 * @throws std::runtime_error On failure to find delimiter in the string.
 */
inline std::pair<std::string, std::string>
splitAt(const std::string &s, char delim)
{
    const std::string::size_type pos = s.find(delim);
    if (pos == std::string::npos) {
        throw std::runtime_error("Can't split " + s + " with " + delim);
    }

    return { s.substr(0, pos), s.substr(pos + 1U) };
}

/**
 * @brief Splits string in a range-for loop friendly way.
 *
 * @param str String to split into substrings.
 * @param with Character to split at.
 *
 * @returns Array of results, empty on empty string.
 */
inline std::vector<std::string>
split(const std::string &str, char with)
{
    if (str.empty()) {
        return {};
    }

    std::vector<std::string> results;
    boost::split(results, str, boost::is_from_range(with, with));
    return results;
}

#endif // UNCOV__UTILS__STRINGS_HPP__
