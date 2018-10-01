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

#ifndef UNCOV__PRINTING_HPP__
#define UNCOV__PRINTING_HPP__

#include <cstddef>
#include <ctime>

#include <memory>
#include <sstream>
#include <string>

/**
 * @file printing.hpp
 *
 * @brief Elements that abstract HTML/ASCII formatting.
 */

class ColorCane;

/**
 * @brief Unit-specific settings.
 */
class PrintingSettings
{
public:
    /**
     * @brief Enable polymorphic destruction.
     */
    virtual ~PrintingSettings() = default;

public:
    /**
     * @brief Retrieves time format string for @c Time printer.
     *
     * @returns The format string.
     */
    virtual std::string getTimeFormat() const = 0;

    /**
     * @brief Retrieves medium limit for @c Coverage printer.
     *
     * Should be between 0.0 and HiLimit.
     *
     * @returns The limit.
     */
    virtual float getMedLimit() const = 0;

    /**
     * @brief Retrieves high limit for @c Coverage printer.
     *
     * Should be greater than or equal to MedLimit.
     *
     * @returns The limit.
     */
    virtual float getHiLimit() const = 0;

    /**
     * @brief Retrieves information about output required format.
     *
     * @returns @c true if output is HTML, @c false otherwise.
     */
    virtual bool isHtmlOutput() const = 0;

public:
    /**
     * @brief Sets settings for the unit.
     *
     * @param settings Pointer to object implementing the settings.
     */
    static void set(std::shared_ptr<PrintingSettings> settings);
};

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

    /**
     * @brief Prints wrapped data in a formatted way (depends on type).
     *
     * Instantiation of this template declares this function.
     *
     * @param cc Output for formatted data.
     * @param w Data Container.
     *
     * @returns @p cc.
     */
    friend ColorCane & operator<<(ColorCane &cc, const PrintWrapper &w);

public:
    /**
     * @brief Initializes data field.
     *
     * @param data Data to be initialized with.
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
    //! Value to be processed (might be a temporary).
    const T &data;
};

//! Strong typing of integer meaning covered lines change.
using CLinesChange = PrintWrapper<int, struct CLinesChangeTag>;

//! Strong typing of integer meaning missed lines change.
using MLinesChange = PrintWrapper<int, struct MLinesChangeTag>;

//! Strong typing of integer meaning relevant lines change.
using RLinesChange = PrintWrapper<int, struct RLinesChangeTag>;

//! Strong typing of float meaning coverage change in percents.
using CoverageChange = PrintWrapper<float, struct CoverageChangeTag>;

//! Strong typing of float meaning coverage in percents.
using Coverage = PrintWrapper<float, struct CoverageTag>;

//! Strong typing of string containing label's title.
using Label = PrintWrapper<std::string, struct LabelTag>;

//! Strong typing of string containing error's title.
using ErrorMsg = PrintWrapper<std::string, struct ErrorMsgTag>;

//! Strong typing of string containing header of a table.
using TableHeader = PrintWrapper<std::string, struct TableHeaderTag>;

//! Information about line number.
struct LineNoInfo {
    LineNoInfo(std::size_t lineNo, int width, bool original = true)
        : lineNo(lineNo), width(width), original(original)
    { }

    std::size_t lineNo; //!< Line number.
    int width;          //!< Minimal width.
    bool original;      //!< Whether this is line number of original side.
};
//! Strong typing of int representing line number.
using LineNo = PrintWrapper<LineNoInfo, struct LineNoTag>;

//! Strong typing of string representing common line in diff.
using LineRetained = PrintWrapper<std::string, struct LineRetainedTag>;

//! Strong typing of string representing added line in diff.
using LineAdded = PrintWrapper<std::string, struct LineAddedTag>;

//! Strong typing of string representing removed line in diff.
using LineRemoved = PrintWrapper<std::string, struct LineRemovedTag>;

//! Strong typing of string representing a note.
using NoteMsg = PrintWrapper<std::string, struct NoteMsgTag>;

//! Information about number of hits.
struct HitsCountInfo {
    int hits;  //!< Number of hits.
    int width; //!< Minimal width.
};
//! Strong typing of int representing number of hits.
using HitsCount = PrintWrapper<HitsCountInfo, struct HitsCountTag>;
//! Int wrapper for number of hits, which shouldn't standout too much.
using SilentHitsCount = PrintWrapper<HitsCountInfo, struct SilentHitsCountTag>;

//! Strong typing of string representing VCS revision.
using Revision = PrintWrapper<std::string, struct RevisionTag>;

//! Strong typing of time representing build timestamp.
using Time = PrintWrapper<std::time_t, struct TimeTag>;

/**
 * @brief Prints ColorCane into a stream.
 *
 * @param os Stream to output formatted data to.
 * @param cc Data to print.
 *
 * @returns @p os.
 */
std::ostream & operator<<(std::ostream &os, const ColorCane &cc);

#endif // UNCOV__PRINTING_HPP__
