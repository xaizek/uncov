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

#ifndef UNCOV__FILEPRINTER_HPP__
#define UNCOV__FILEPRINTER_HPP__

#include <srchilite/sourcehighlight.h>
#include <srchilite/langmap.h>

#include <iosfwd>
#include <string>
#include <vector>

/**
 * @file FilePrinter.hpp
 *
 * @brief Printing files or their diffs annotated with coverage information.
 */

class FileComparator;

/**
 * @brief FilePrinter-specific settings.
 */
class FilePrinterSettings
{
public:
    /**
     * @brief Enable polymorphic destruction.
     */
    virtual ~FilePrinterSettings() = default;

public:
    /**
     * @brief Retrieves number of spaces that comprise a tabulation.
     *
     * @returns The number.
     */
    virtual int getTabSize() const = 0;

    /**
     * @brief Retrieves information about availability of color processing.
     *
     * @returns @c true if output can be colorized, @c false otherwise.
     */
    virtual bool isColorOutputAllowed() const = 0;

    /**
     * @brief Retrieves information about required output format.
     *
     * @returns @c true if output is HTML, @c false otherwise.
     */
    virtual bool isHtmlOutput() const = 0;
};

/**
 * @brief Prints highlighted files or their diffs annotated with coverage.
 */
class FilePrinter
{
public:
    /**
     * @brief Constructs an object performing some highlighting preparations.
     *
     * @param settings FilePrinter settings.
     * @param allowColors Highlight source file.
     */
    explicit FilePrinter(const FilePrinterSettings &settings,
                         bool allowColors = true);

public:
    /**
     * @brief Prints highlighted file with optional folding of covered lines.
     *
     * @param os Stream to print output to.
     * @param path Name of the file (for highlighting detection).
     * @param contents Contents of the file.
     * @param coverage Coverage information per line of contents.
     * @param leaveMissedOnly Fold lines which are covered or not relevant.
     *
     * @note @c coverage.size() should match lines in @p contents.
     */
    void print(std::ostream &os, const std::string &path,
               const std::string &contents, const std::vector<int> &coverage,
               bool leaveMissedOnly = false);

    /**
     * @brief Finds and prints differences between two versions of a file.
     *
     * Implements solution for longest common subsequence problem that matches
     * modified finding of edit distance (substitution operation excluded) with
     * backtracking afterward to compose result.  Requires lots of memory for
     * very big files.
     *
     * @param os Stream to print output to.
     * @param path Name of the file (for highlighting detection).
     * @param oText Old version of the file.
     * @param oCov Coverage of old version.
     * @param nText New version of the file.
     * @param nCov Coverage of new version.
     * @param comparator Object with loaded file diffing results.
     *
     * @note `oText.size() == oCov.size() && nText.size() == nCov.size()` is
     *       assumed.
     */
    void printDiff(std::ostream &os, const std::string &path,
                   std::istream &oText, const std::vector<int> &oCov,
                   std::istream &nText, const std::vector<int> &nCov,
                   const FileComparator &comparator);

private:
    /**
     * @brief Derives language from path.
     *
     * @param path Path to analyze.
     *
     * @returns Determined language.
     */
    std::string getLang(const std::string &path);
    /**
     * @brief Highlights source code.
     *
     * @param ss Stream for highlighted output.
     * @param text Code to highlight.
     * @param lang Language in which the code is written.
     * @param ranges Ranges of lines to be highlighted.
     */
    void highlight(std::stringstream &ss, std::istream &text,
                   const std::string &lang,
                   srchilite::LineRanges *ranges = nullptr);

private:
    //! Whether code highlighting is enabled.
    const bool colorizeOutput;
    //! Code highlighting object.
    srchilite::SourceHighlight highlighter;
    //! Loaded language map.
    srchilite::LangMap langMap;
};

#endif // UNCOV__FILEPRINTER_HPP__
