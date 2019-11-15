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

#ifndef UNCOV__UTILS__MEMORY_HPP__
#define UNCOV__UTILS__MEMORY_HPP__

#include <memory>
#include <utility>

/**
 * @file memory.hpp
 *
 * @brief This unit implements make_unique from C++14 standard.
 */

/**
 * @brief make_unique function missing in C++11.
 *
 * @tparam T Type of object to construct.
 * @tparam Args Types of constructor arguments.
 * @param args Actual arguments for the constructor.
 *
 * @returns Unique pointer with newly constructed object.
 */
template <typename T, typename... Args>
inline std::unique_ptr<T>
make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

#endif // UNCOV__UTILS__MEMORY_HPP__
