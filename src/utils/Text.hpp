// Copyright (C) 2016 xaizek <xaizek@posteo.net>
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

#ifndef UNCOV__UTILS__TEXT_HPP__
#define UNCOV__UTILS__TEXT_HPP__

#include <cstddef>

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

/**
 * @file Text.hpp
 *
 * @brief A text handling class that exposes it as lines or a stream.
 */

/**
 * @brief A convenient class that provides access to text in different forms.
 */
class Text
{
public:
    /**
     * @brief Initializes the text object from multiline string.
     *
     * @param text Multiline string.
     */
    Text(const std::string &text) : iss(text)
    {
        lines.reserve(std::count(text.cbegin(), text.cend(), '\n') + 1);
        for (std::string line; std::getline(iss, line); ) {
            lines.push_back(line);
        }
    }

public:
    /**
     * @brief Retrieves the text as vector of lines.
     *
     * @returns The vector of text lines.
     */
    const std::vector<std::string> & asLines() const
    {
        return lines;
    }

    /**
     * @brief Retrieves the text as a stream.
     *
     * At most one use at a time.
     *
     * @returns Returns the stream.
     */
    std::istream & asStream()
    {
        iss.clear();
        iss.seekg(0);
        return iss;
    }

    /**
     * @brief Retrieves size of the text in lines.
     *
     * @returns The size.
     */
    std::size_t size()
    {
        return lines.size();
    }

private:
    //! Storage of text.
    std::istringstream iss;
    //! Text broken in lines.
    std::vector<std::string> lines;
};

#endif // UNCOV__UTILS__TEXT_HPP__
