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

#ifndef UNCOV__UTILS__MD5_HPP__
#define UNCOV__UTILS__MD5_HPP__

#include <string>

/**
 * @file md5.hpp
 *
 * @brief A third-party implementation of MD5 algorithm.
 */

/**
 * @brief Computes hash of the string.
 *
 * @param str String to hash.
 *
 * @returns MD5 hash of the string.
 */
std::string md5(const std::string &str);

#endif // UNCOV__UTILS__MD5_HPP__
