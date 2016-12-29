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

#ifndef UNCOV__UTILS__FS_HPP__
#define UNCOV__UTILS__FS_HPP__

#include <boost/filesystem/path.hpp>

/**
 * @brief Checks that @path is somewhere under @root.
 *
 * @param root Root to check against.
 * @param path Path to check.
 *
 * @returns @c true if so, otherwise @c false.
 *
 * @note Path are assumed to be canonicalized.
 */
bool pathIsInSubtree(const boost::filesystem::path &root,
                     const boost::filesystem::path &path);

#endif // UNCOV__UTILS__FS_HPP__
