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

#include "utils/fs.hpp"

#include <boost/filesystem/path.hpp>

#include <algorithm>
#include <iterator>

namespace fs = boost::filesystem;

bool
pathIsInSubtree(const fs::path &root, const fs::path &path)
{
    auto rootLen = std::distance(root.begin(), root.end());
    auto pathLen = std::distance(path.begin(), path.end());
    if (pathLen < rootLen) {
        return false;
    }

    return std::equal(root.begin(), root.end(), path.begin());
}
