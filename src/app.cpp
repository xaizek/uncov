// Copyright (C) 2021 xaizek <xaizek@posteo.net>
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

#include "app.hpp"

#include <boost/filesystem/operations.hpp>

#include <cassert>

#include <string>
#include <vector>

#include "Repository.hpp"

static const std::string configFileName = "uncov.ini";
static const std::string databaseFileName = "uncov.sqlite";

std::string getAppVersion()
{
    return "v0.4";
}

std::string getConfigFile()
{
    return configFileName;
}

std::string getDatabaseFile()
{
    return databaseFileName;
}

std::string
pickDataPath(const Repository &repo)
{
    namespace fs = boost::filesystem;

    std::vector<std::string> paths = repo.getGitPaths();
    assert(!paths.empty() && "Must be at least one path.");

    // Find the closest directory where uncov is configured.
    for (const auto &path : paths) {
        if (fs::exists(path + '/' + configFileName) ||
            fs::exists(path + '/' + databaseFileName)) {
            return path;
        }
    }

    // Or return the most generic one.
    return paths.back();
}
