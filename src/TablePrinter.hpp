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

#ifndef UNCOV_TABLEPRINTER_HPP_
#define UNCOV_TABLEPRINTER_HPP_

#include <iosfwd>
#include <string>
#include <vector>

/**
 * @file TablePrinter.hpp
 *
 * @brief This unit provides table formatting facilities for terminal.
 */

/**
 * @brief String table formatter and printer.
 *
 * Format and sorting are configurable via constructor's parameters.
 */
class TablePrinter
{
    class Column;

public:
    /**
     * @brief Constructs the table formatter.
     *
     * @param headings Format specification: field, field...
     * @param maxWidth Maximum allowed table width.
     * @param hiddenHeader Do not print table header.
     */
    TablePrinter(const std::vector<std::string> &headings,
                 unsigned int maxWidth, bool hiddenHeader = false);
    /**
     * @brief To emit destructing code in corresponding source file.
     */
    ~TablePrinter();

public:
    /**
     * @brief Adds item to the table.
     *
     * @param item Row to add.
     *
     * @throws std::invalid_argument if item length doesn't match columns.
     */
    void append(const std::vector<std::string> &item);
    /**
     * @brief Prints table on standard output.
     *
     * @param os Output stream.
     */
    void print(std::ostream &os);

private:
    /**
     * @brief Populates columns with items' data.
     */
    void fillColumns();
    /**
     * @brief Ensures that columns fit into required width limit.
     *
     * @returns @c true on successful shrinking or @c false on failure.
     */
    bool adjustColumnsWidths();
    /**
     * @brief Print table heading.
     *
     * @param os Output stream.
     */
    void printTableHeader(std::ostream &os);
    /**
     * @brief Prints table lines.
     *
     * @param os Output stream.
     */
    void printTableRows(std::ostream &os);
    /**
     * @brief Pads string to align it according to column parameters.
     *
     * We can't simply use @c std::setw, because of escape sequences.
     *
     * @param s String to align.
     * @param col Source of width and alignment type.
     *
     * @returns Aligned cell.
     */
    std::string alignCell(std::string s, const Column &col) const;

private:
    //! Maximum allowed table width.
    const unsigned int maxWidth;
    //! Whether header should be omitted in output.
    bool hiddenHeader;
    //! List of columns of the table.
    std::vector<Column> cols;
    //! List of items to display.
    std::vector<std::vector<std::string>> items;
};

#endif // UNCOV_TABLEPRINTER_HPP_
