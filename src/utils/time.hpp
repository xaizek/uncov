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

#ifndef UNCOV_UTILS_TIME_HPP_
#define UNCOV_UTILS_TIME_HPP_

#include <ctime>

#include <ostream>
#include <tuple>

/**
 * @file time.hpp
 *
 * @brief This unit implements put_time from C++14 standard.
 */

//! std::put_time emulation with a tuple (available since GCC 5.0).
using put_time = std::tuple<const std::tm *, const char *>;

/**
 * @brief Expands put_time data into a string and prints it out.
 *
 * @param os Stream to print formated time onto.
 * @param pt put_time manipulator emulation.
 *
 * @returns @p os.
 */
inline std::ostream &
operator<<(std::ostream &os, const put_time &pt)
{
    char buf[128];
    std::strftime(buf, sizeof(buf), std::get<1>(pt), std::get<0>(pt));
    return os << buf;
}

#endif // UNCOV_UTILS_TIME_HPP_
