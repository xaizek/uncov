// Copyright (C) 2018 xaizek <xaizek@posteo.net>
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

#include "GcovImporter.hpp"

#include <boost/algorithm/string/trim.hpp>
#include <boost/filesystem/operations.hpp>

#include <fstream>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>

#include "utils/fs.hpp"
#include "utils/md5.hpp"
#include "utils/strings.hpp"
#include "BuildHistory.hpp"

namespace fs = boost::filesystem;

std::function<GcovImporter::runner_f> GcovImporter::runner;

void
GcovImporter::setRunner(std::function<runner_f> runner)
{
    GcovImporter::runner = std::move(runner);
}

GcovImporter::GcovImporter(const std::string &root,
                           const std::string &covoutRoot,
                           const std::vector<std::string> &exclude)
    : rootDir(normalizePath(fs::absolute(root)))
{
    for (const fs::path &p : exclude) {
        skipPaths.insert(normalizePath(fs::absolute(p, root)));
    }

    std::unordered_set<std::string> extensions = {
        ".h", ".hh", ".hpp", ".hxx",
        ".c", ".cc", ".cpp", ".cxx",
        ".m", ".mm",
    };
    std::unordered_set<std::string> skipDirs = {
        ".git", ".hg", ".svn", // Various version control systems.
        ".deps"                // Dependency tracking of automake.
    };

    std::vector<std::string> cmd = {
        "gcov", "--preserve-paths", "--intermediate-format", "--"
    };

    for (fs::recursive_directory_iterator it(fs::absolute(covoutRoot)), end;
         it != end; ++it) {
        fs::path path = it->path();
        if (fs::is_directory(path)) {
            if (skipDirs.count(path.filename().string())) {
                it.no_push();
                continue;
            }
        } else if (fs::is_regular(path)) {
            // We are looking for *.gcno and not *.gcda, because they are
            // generated even for files that weren't executed (e.g.,
            // `main.cpp`).
            if (path.extension() == ".gcno") {
                cmd.emplace_back(path.string());
            }
        }
    }

    TempDir tempDir("gcovi");
    std::string tempDirPath = tempDir;
    runner(std::move(cmd), tempDirPath);

    for (fs::directory_entry &e :
         fs::recursive_directory_iterator(tempDirPath)) {
        fs::path path = e.path();
        if (fs::is_regular(path) && path.extension() == ".gcov") {
            parseGcov(path.string());
        }
    }

    for (fs::recursive_directory_iterator it(rootDir), end; it != end; ++it) {
        fs::path path = it->path();
        if (fs::is_directory(path)) {
            if (skipDirs.count(path.filename().string())) {
                it.no_push();
            } else if (skipPaths.count(normalizePath(fs::absolute(path)))) {
                it.no_push();
            }
        } else if (extensions.count(path.extension().string())) {
            std::string filePath = makeRelativePath(rootDir, path).string();
            if (mapping.find(filePath) == mapping.end()) {
                std::string contents = readFile(path.string());
                std::string hash = md5(contents);

                int nLines = std::count(contents.cbegin(), contents.cend(),
                                        '\n');

                files.emplace_back(filePath, std::move(hash),
                                   std::vector<int>(nLines, -1));
            }
        }
    }

    for (auto &e : mapping) {
        std::string contents = readFile(root + '/' + e.first);
        std::string hash = md5(contents);

        std::vector<std::string> lines = split(contents, '\n');
        e.second.resize(lines.size(), -1);

        unsigned int i = 0U;
        for (std::string &line : lines) {
            boost::trim_if(line, boost::is_any_of("\r\n \t"));
            if (line == "}" || line == "};") {
                e.second[i] = -1;
            }
            ++i;
        }

        files.emplace_back(e.first, std::move(hash), std::move(e.second));
    }

    mapping.clear();
}

std::vector<File> &&
GcovImporter::getFiles() &&
{
    return std::move(files);
}

void
GcovImporter::parseGcov(const std::string &path)
{
    std::vector<int> *coverage = nullptr;
    std::ifstream in(path);
    for (std::string line; in >> line; ) {
        std::string type, value;
        std::tie(type, value) = splitAt(line, ':');
        if (type == "file") {
            coverage = nullptr;

            fs::path sourcePath = normalizePath(fs::absolute(value, rootDir));
            if (!pathIsInSubtree(rootDir, sourcePath) ||
                isExcluded(sourcePath)) {
                continue;
            }

            sourcePath = makeRelativePath(rootDir, sourcePath);
            coverage = &mapping[sourcePath.string()];
        } else if (coverage != nullptr && type == "lcount") {
            std::vector<std::string> fields = split(value, ',');
            if (fields.size() < 2U) {
                throw std::runtime_error("Not enough fields in lcount: " +
                                         value);
            }

            unsigned int lineNo = std::stoi(fields[0]);
            int count = std::stoi(fields[1]);

            if (coverage->size() < lineNo) {
                coverage->resize(lineNo, -1);
            }

            int &entry = (*coverage)[lineNo - 1U];
            entry = (entry == -1 ? count : entry + count);
        }
    }
}

bool
GcovImporter::isExcluded(boost::filesystem::path path) const
{
    for (const fs::path &skipPath : skipPaths) {
        if (pathIsInSubtree(skipPath, path)) {
            return true;
        }
    }
    return false;
}
