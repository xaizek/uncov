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

#ifndef UNCOVER__PRINTING_HPP__
#define UNCOVER__PRINTING_HPP__

#include <cstddef>
#include <ctime>

#include <sstream>
#include <string>

/**
 * @brief Wrapper to enable multiple overloads for the same type of data.
 *
 * Should be used only as a temporary object as it doesn't copying passed in
 * data.
 *
 * @tparam T Type of the contained data.
 * @tparam C Tag structure to enforce creation of different types.
 */
template <typename T, typename C>
class PrintWrapper
{
    /**
     * @brief Prints wrapped data in a formatted way (depends on type).
     *
     * Instantiation of this template declares this function.
     *
     * @param os Stream to output formatted data to.
     * @param w Data Container.
     *
     * @returns @p os.
     */
    friend std::ostream & operator<<(std::ostream &os, const PrintWrapper &w);

public:
    /**
     * @brief Initializes data field.
     *
     * @param d Data to be initialized with.
     */
    PrintWrapper(const T &data) : data(data) {}

public:
    /**
     * @brief Prints data into a string.
     *
     * @returns The printed data.
     */
    operator std::string() const
    {
        std::ostringstream oss;
        oss << *this;
        return oss.str();
    }

private:
    /**
     * @brief Value to be processed (might be a temporary).
     */
    const T &data;
};

/**
 * @brief Strong typing of integer meaning covered lines change.
 */
using CLinesChange = PrintWrapper<int, struct CLinesChangeTag>;

/**
 * @brief Strong typing of integer meaning missed lines change.
 */
using MLinesChange = PrintWrapper<int, struct MLinesChangeTag>;

/**
 * @brief Strong typing of integer meaning relevant lines change.
 */
using RLinesChange = PrintWrapper<int, struct RLinesChangeTag>;

/**
 * @brief Strong typing of float meaning coverage change in percents.
 */
using CoverageChange = PrintWrapper<float, struct CoverageChangeTag>;

/**
 * @brief Strong typing of float meaning coverage in percents.
 */
using Coverage = PrintWrapper<float, struct CoverageTag>;

/**
 * @brief Strong typing of string containing label's title.
 */
using Label = PrintWrapper<std::string, struct LabelTag>;

/**
 * @brief Strong typing of string containing error's title.
 */
using ErrorMsg = PrintWrapper<std::string, struct ErrorMsgTag>;

/**
 * @brief Strong typing of string containing header of a table.
 */
using TableHeader = PrintWrapper<std::string, struct TableHeaderTag>;

/**
 * @brief Strong typing of int representing line number.
 */
using LineNo = PrintWrapper<std::size_t, struct LineNoTag>;

/**
 * @brief Strong typing of string representing added line.
 */
using LineAdded = PrintWrapper<std::string, struct LineAddedTag>;

/**
 * @brief Strong typing of string representing removed line.
 */
using LineRemoved = PrintWrapper<std::string, struct LineRemovedTag>;

/**
 * @brief Strong typing of int representing number of hits.
 */
using HitsCount = PrintWrapper<int, struct HitsCountTag>;

/**
 * @brief Int wrapper for number of hits, which shouldn't standout too much.
 */
using SilentHitsCount = PrintWrapper<int, struct SilentHitsCountTag>;

/**
 * @brief Strong typing of string representing VCS revision.
 */
using Revision = PrintWrapper<std::string, struct RevisionTag>;

/**
 * @brief Strong typing of time representing build timestamp.
 */
using Time = PrintWrapper<std::time_t, struct TimeTag>;

#endif // UNCOVER__PRINTING_HPP__
