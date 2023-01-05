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

#ifndef UNCOV_UTILS_FLAG_HPP_
#define UNCOV_UTILS_FLAG_HPP_

/**
 * @file Flag.hpp
 *
 * @brief Strong typedef for boolean flags.
 */

/**
 * @brief Type safe boolean flag.
 *
 * Usage example:
 * @code
 * // type declaration
 * using DoSpacing = Flag<struct DoSpacingTag>;
 * // type use
 * void printSomething(DoSpacing spacing);
 * // value use
 * printSomething(DoSpacing{});  // read "do spacing"
 * printSomething(!DoSpacing{}); // read as "don't do spacing"
 * @endcode
 *
 * Supposed to be zero-overhead.
 *
 * @tparam Tag Tag structure to enforce creation of different types.
 */
template <typename Tag>
class Flag
{
public:
    /**
     * @brief Constructs a flag (set by default).
     *
     * @param value Flag value.
     */
    explicit constexpr Flag(bool value = true) : value(value) {}

public:
    /**
     * @brief Inverts value of the flag.
     *
     * @returns New flag holding inverted value.
     */
    constexpr Flag operator!() const
    {
        return Flag(!value);
    }

    /**
     * @brief Implicitly casts the flag to boolean value.
     *
     * @returns Value of the flag.
     */
    constexpr operator bool() const
    {
        return value;
    }

private:
    //! Value of the flag.
    const bool value;
};

#endif // UNCOV_UTILS_FLAG_HPP_
