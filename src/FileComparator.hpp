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

#ifndef UNCOV__FILECOMPARATOR_HPP__
#define UNCOV__FILECOMPARATOR_HPP__

#include <deque>
#include <string>
#include <utility>
#include <vector>

/**
 * @file FileComparator.hpp
 *
 * @brief This unit provides file comparison functionality.
 */

/**
 * @brief Type of a single line of a diff.
 */
enum class DiffLineType
{
    Note,      //!< Textual note.
    Common,    //!< Line with non-essential changes.
    Identical, //!< Identical line.
    Added,     //!< New line added.
    Removed    //!< Old line removed.
};

/**
 * @brief Type of file comparison.
 */
enum class CompareStrategy
{
    State,   //!< Compare lines by states (covered, not covered, not relevant).
    Hits,    //!< Compare different number of hits as different.
    Regress, //!< Display new not covered and old previously covered lines.
};

/**
 * @brief Single line of a diff.
 */
struct DiffLine
{
    DiffLineType type; //!< Type of this diff line.
    std::string text;  //!< Note text for DiffLineType::Note type.
    int oldLine;       //!< Index of line in old version if makes sense, or -1.
    int newLine;       //!< Index of line in new version if makes sense, or -1.

    /**
     * @brief Constructs diff by initializing all the fields.
     *
     * The constructor is needed to be able to use emplace().
     *
     * @param type    @copybrief type
     * @param text    @copybrief text
     * @param oldLine @copybrief oldLine
     * @param newLine @copybrief newLine
     */
    DiffLine(DiffLineType type, std::string text,
             int oldLine = -1, int newLine = -1)
        : type(type), text(std::move(text)), oldLine(oldLine), newLine(newLine)
    {
    }

    /**
     * @brief Retrieves active line number (either oldLine or newLine).
     *
     * @returns The line number.
     */
    int getLine() const
    {
        return oldLine > newLine ? oldLine : newLine;
    }
};

/**
 * @brief FileComparator-specific settings.
 */
class FileComparatorSettings
{
public:
    /**
     * @brief Enable polymorphic destruction.
     */
    virtual ~FileComparatorSettings() = default;

public:
    /**
     * @brief Retrieves minimal size of a fold.
     *
     * @returns The size.
     */
    virtual int getMinFoldSize() const = 0;

    /**
     * @brief Retrieves size of context in diff results.
     *
     * Both above and below folded piece.
     *
     * @returns The size.
     */
    virtual int getFoldContext() const = 0;
};

/**
 * @brief Generates diff of both lines and coverage.
 */
class FileComparator
{
public:
    /**
     * @brief Constructs an instance validating data arguments.
     *
     * @param o        Old lines.
     * @param oCov     Coverage of old lines.
     * @param n        New lines.
     * @param nCov     Coverage of new lines.
     * @param strategy Comparison strategy.
     * @param settings Settings for tweaking the comparison.
     */
    FileComparator(const std::vector<std::string> &o,
                   const std::vector<int> &oCov,
                   const std::vector<std::string> &n,
                   const std::vector<int> &nCov,
                   CompareStrategy strategy,
                   const FileComparatorSettings &settings);

public:
    /**
     * @brief Retrieves whether data passed into constructor was valid.
     *
     * @returns @c true if so, @c false otherwise.
     */
    bool isValidInput() const;
    /**
     * @brief Retrieves error description when input wasn't valid.
     *
     * @returns Error message.
     */
    std::string getInputError() const;
    /**
     * @brief Retrieves whether old and new states are equal.
     *
     * @returns @c true if so, @c false otherwise.
     */
    bool areEqual() const;
    /**
     * @brief Retrieves generated diff sequence.
     *
     * @returns The sequence.
     */
    const std::deque<DiffLine> & getDiffSequence() const;

private:
    bool valid;                   //!< Whether passed in data was valid.
    std::string inputError;       //!< Error message describing what's wrong.
    bool equal;                   //!< Whether old and new states match.
    std::deque<DiffLine> diffSeq; //!< Generated diff output.
};

#endif // UNCOV__FILECOMPARATOR_HPP__
