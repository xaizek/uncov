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

#ifndef UNCOV__DECORATION_HPP__
#define UNCOV__DECORATION_HPP__

#include <memory>
#include <iosfwd>
#include <vector>

/**
 * @file decoration.hpp
 *
 * @brief Facilities for decorating text with escape-sequences.
 */

/**
 * @brief Terminal control manipulators for output streams.
 *
 * Usage example:
 * @code
 * std::cout << decor::bold << "This is bold text." << decor::def;
 * @endcode
 */
namespace decor {

/**
 * @brief Type of function that performs decoration.
 */
using decorFunc = std::ostream & (*)(std::ostream &os);

/**
 * @brief Class describing single decoration or a combination of them.
 */
class Decoration
{
public:
    /**
     * @brief Constructs empty decoration.
     */
    Decoration() = default;
    /**
     * @brief Constructs a (deep) copy of a decoration.
     *
     * @param rhs Original object to be copied.
     */
    Decoration(const Decoration &rhs);
    /**
     * @brief Constructs decoration from a function.
     *
     * @param decorator Decorating function.
     */
    explicit Decoration(decorFunc decorator);
    /**
     * @brief Constructs a decoration that is a combination of others.
     *
     * @param lhs First decoration to combine.
     * @param rhs Second decoration to combine.
     */
    Decoration(const Decoration &lhs, const Decoration &rhs);
    /**
     * @brief Move-assigns object's value.
     *
     * @param rhs Source of the move-assignment operation.
     *
     * @returns @c *this
     */
    Decoration & operator=(Decoration &&rhs) = default;

public:
    /**
     * @brief Actually performs the decoration of a stream.
     *
     * @param os Stream to decorate.
     *
     * @returns @p os.
     */
    std::ostream & decorate(std::ostream &os) const;

private:
    //! Decoration function (can be nullptr).
    decorFunc decorator = nullptr;
    //! One of two decorations that compose this object.
    std::unique_ptr<Decoration> lhs;
    //! Second decoration that composes this object.
    std::unique_ptr<Decoration> rhs;
};

/**
 * @brief Combines two decorations to create a new one.
 *
 * @param lhs First decoration to combine.
 * @param rhs Second decoration to combine.
 *
 * @returns The combination.
 */
inline Decoration
operator+(const Decoration &lhs, const Decoration &rhs)
{
    return Decoration(lhs, rhs);
}

/**
 * @brief A thunk for decoration use with @c std::ostream.
 *
 * @param os Stream to output to.
 * @param d Decoration to apply.
 *
 * @returns @p os.
 */
inline std::ostream &
operator<<(std::ostream &os, const Decoration &d)
{
    return d.decorate(os);
}

/**
 * @{
 * @name Generic attributes
 */

extern const Decoration none; //!< Convenience attribute that does nothing.
extern const Decoration bold; //!< Enables bold attribute.
extern const Decoration inv;  //!< Enables color inversion attribute.
extern const Decoration def;  //!< Restores default attribute of the stream.

/**
 * @}
 *
 * @{
 * @name Foreground colors
 */

extern const Decoration black_fg;   //!< Picks black as foreground color.
extern const Decoration red_fg;     //!< Picks red as foreground color.
extern const Decoration green_fg;   //!< Picks green as foreground color.
extern const Decoration yellow_fg;  //!< Picks yellow as foreground color.
extern const Decoration blue_fg;    //!< Picks blue as foreground color.
extern const Decoration magenta_fg; //!< Picks magenta as foreground color.
extern const Decoration cyan_fg;    //!< Picks cyan as foreground color.
extern const Decoration white_fg;   //!< Picks white as foreground color.

/**
 * @}
 *
 * @{
 * @name Background colors
 */

extern const Decoration black_bg;   //!< Picks black as background color.
extern const Decoration red_bg;     //!< Picks red as background color.
extern const Decoration green_bg;   //!< Picks green as background color.
extern const Decoration yellow_bg;  //!< Picks yellow as background color.
extern const Decoration blue_bg;    //!< Picks blue as background color.
extern const Decoration magenta_bg; //!< Picks magenta as background color.
extern const Decoration cyan_bg;    //!< Picks cyan as background color.
extern const Decoration white_bg;   //!< Picks white as background color.

/**
 * @}
 *
 * @{
 * @name Control
 */

/**
 * @brief Forces disabling of decorations.
 */
void disableDecorations();

/**
 * @}
 */

}

#endif // UNCOV__DECORATION_HPP__
