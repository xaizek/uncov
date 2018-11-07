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

#include "utils/fs.hpp"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include <algorithm>
#include <fstream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>

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

fs::path
normalizePath(const fs::path &path)
{
    fs::path result;
    for (fs::path::iterator it = path.begin(); it != path.end(); ++it) {
        if (*it == "..") {
            if(result.filename() == "..") {
                result /= *it;
            } else {
                result = result.parent_path();
            }
        } else if (*it != ".") {
            result /= *it;
        }
    }
    return result;
}

fs::path
makeRelativePath(fs::path base, fs::path path)
{
    auto baseIt = base.begin();
    auto pathIt = path.begin();

    while (baseIt != base.end() && pathIt != path.end() && *pathIt == *baseIt) {
        ++pathIt;
        ++baseIt;
    }

    fs::path finalPath;
    while (baseIt != base.end()) {
        finalPath /= "..";
        ++baseIt;
    }

    while (pathIt != path.end()) {
        finalPath /= *pathIt;
        ++pathIt;
    }

    return finalPath;
}

std::string
readFile(const std::string &path)
{
    if (fs::is_directory(path)) {
        throw std::runtime_error("Not a regular file: " + path);
    }

    std::ifstream ifile(path);
    if (!ifile) {
        throw std::runtime_error("Can't open file: " + path);
    }

    std::ostringstream iss;
    iss << ifile.rdbuf();
    return iss.str();
}
