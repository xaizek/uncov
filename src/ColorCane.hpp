// Copyright (C) 2018 xaizek <xaizek@posteo.net>
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

#ifndef UNCOV__COLORCANE_HPP__
#define UNCOV__COLORCANE_HPP__

#include <boost/utility/string_ref.hpp>

#include <string>
#include <utility>
#include <vector>

enum class ColorGroup;

/**
 * @brief Single item of a ColorCane.
 */
struct ColorCanePiece
{
    /**
     * @brief Constructs the item.
     *
     * @param text Contents of the item (might be empty).
     * @param hi   Highlighting group of the item.
     */
    ColorCanePiece(std::string text, ColorGroup hi)
        : text(std::move(text)), hi(hi)
    { }

    std::string text; //!< Text of the item (can be empty).
    ColorGroup hi;    //!< Highlighting of the piece.
};

/**
 * @brief Allows constructing string consisting of multiple pieces each of which
 *        is associated with some metadata.
 */
class ColorCane
{
    /**
     * @brief Type of collection of pieces.
     */
    using Pieces = std::vector<ColorCanePiece>;

public:
    /**
     * @brief Appends a string.
     *
     * @param text Text to append.
     * @param hi   Highlighting group of the text.
     */
    void append(boost::string_ref text, ColorGroup hi = {});
    /**
     * @brief Appends single character.
     *
     * @param text Character to append.
     * @param hi   Highlighting group of the text.
     */
    void append(char text, ColorGroup hi = {});

    /**
     * @brief Retrieves beginning of the list of pieces.
     *
     * @returns The iterator.
     */
    Pieces::const_iterator begin() const;
    /**
     * @brief Retrieves end of the list of pieces.
     *
     * @returns The iterator.
     */
    Pieces::const_iterator end() const;

private:
    Pieces pieces; //!< Collection of pieces.
};

#endif // UNCOV__COLORCANE_HPP__
