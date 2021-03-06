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

#include "listings.hpp"

#include <boost/filesystem/path.hpp>
#include <boost/optional.hpp>

#include <map>
#include <ostream>
#include <string>
#include <vector>

#include "utils/fs.hpp"
#include "BuildHistory.hpp"
#include "coverage.hpp"
#include "printing.hpp"

static std::map<std::string, CovInfo>
getDirsCoverage(const Build &build, const std::string &dirFilter);
static CovChange getBuildCovChange(BuildHistory *bh, const Build &build,
                                   const CovInfo &covInfo,
                                   const Build *prevBuild = nullptr);
static void printFileHeader(std::ostream &os, const std::string &filePath,
                            const CovInfo &covInfo, const CovChange &covChange);
static CovChange getFileCovChange(BuildHistory *bh, const Build &build,
                                  const std::string &path,
                                  boost::optional<Build> *prevBuildHint,
                                  const CovInfo &covInfo,
                                  const Build *prevBuild = nullptr);

std::vector<std::string>
describeBuild(BuildHistory *bh, const Build &build, DoExtraAlign extraAlign,
              DoSpacing spacing, const Build *prevBuild)
{
    const std::string sep = spacing ? " / " : "/";
    CovInfo covInfo(build);
    CovChange covChange = getBuildCovChange(bh, build, covInfo, prevBuild);

    return {
        "#" + std::to_string(build.getId()),
        covInfo.formatCoverageRate(),
        covInfo.formatLines(sep),
        covChange.formatCoverageRate(),
        covChange.formatLines(sep, extraAlign ? 4 : 0),
        build.getRefName(),
        Revision{build.getRef()},
        Time{build.getTimestamp()}
    };
}

std::vector<std::vector<std::string>>
describeBuildDirs(BuildHistory *bh, const Build &build,
                  const std::string &dirFilter, const Build *prevBuild)
{
    std::map<std::string, CovInfo> newDirs = getDirsCoverage(build, dirFilter);

    std::map<std::string, CovInfo> prevDirs;
    if (prevBuild != nullptr) {
        prevDirs = getDirsCoverage(*prevBuild, dirFilter);
    } else if (const int prevBuildId = bh->getPreviousBuildId(build.getId())) {
        prevDirs = getDirsCoverage(*bh->getBuild(prevBuildId), dirFilter);
    }

    std::vector<std::vector<std::string>> rows;
    rows.reserve(newDirs.size());

    for (const auto &entry : newDirs) {
        const CovInfo &covInfo = entry.second;
        CovChange covChange(prevDirs[entry.first], covInfo);

        rows.push_back({
            entry.first + '/',
            covInfo.formatCoverageRate(),
            covInfo.formatLines(" / "),
            covChange.formatCoverageRate(),
            covChange.formatLines(" / ", 4)
        });
    }

    return rows;
}

static std::map<std::string, CovInfo>
getDirsCoverage(const Build &build, const std::string &dirFilter)
{
    std::map<std::string, CovInfo> dirs;
    for (const std::string &filePath : build.getPaths()) {
        if (!pathIsInSubtree(dirFilter, filePath)) {
            continue;
        }

        boost::filesystem::path dirPath = filePath;
        dirPath.remove_filename();

        File &file = *build.getFile(filePath);
        CovInfo &info = dirs[dirPath.string()];

        info.add(CovInfo(file));
    }
    return dirs;
}

std::vector<std::vector<std::string>>
describeBuildFiles(BuildHistory *bh, const Build &build,
                   const std::string &dirFilter, ListChangedOnly changedOnly,
                   ListDirectOnly directOnly, const Build *prevBuild)
{
    const std::vector<std::string> &paths = build.getPaths();

    std::vector<std::vector<std::string>> rows;
    rows.reserve(paths.size());

    boost::optional<Build> prev;
    if (prevBuild != nullptr) {
        prev = *prevBuild;
    } else if (const int prevBuildId = bh->getPreviousBuildId(build.getId())) {
        prev = bh->getBuild(prevBuildId);
    }

    boost::filesystem::path dpath = dirFilter;
    for (const std::string &filePath : paths) {
        boost::filesystem::path fpath = filePath;

        if (!pathIsInSubtree(dpath, fpath)) {
            continue;
        }

        if (directOnly && fpath.parent_path() != dpath) {
            continue;
        }

        CovInfo covInfo(*build.getFile(filePath));
        CovChange covChange = getFileCovChange(bh, build, filePath, &prev,
                                               covInfo);

        if (changedOnly && !covChange.isChanged()) {
            continue;
        }

        rows.push_back({
            directOnly ? fpath.filename().string() : filePath,
            covInfo.formatCoverageRate(),
            covInfo.formatLines(" / "),
            covChange.formatCoverageRate(),
            covChange.formatLines(" / ", 4),
        });
    }

    return rows;
}

void
printBuildHeader(std::ostream &os, BuildHistory *bh, const Build &build,
                 const Build *prevBuild)
{
    std::vector<std::string> v = describeBuild(bh, build, DoExtraAlign{},
                                               !DoSpacing{}, prevBuild);
    os << Label{"Build"} << ": " << v[0] << ", "
       << v[1] << '(' << v[2] << "), "
       << v[3] << '(' << v[4] << "), "
       << v[5] << '\n';
}

static CovChange
getBuildCovChange(BuildHistory *bh, const Build &build, const CovInfo &covInfo,
                  const Build *prevBuild)
{
    CovInfo prevCovInfo;
    if (prevBuild != nullptr) {
        prevCovInfo = CovInfo(*prevBuild);
    } else if (const int prevBuildId = bh->getPreviousBuildId(build.getId())) {
        prevCovInfo = CovInfo(*bh->getBuild(prevBuildId));
    }

    return CovChange(prevCovInfo, covInfo);
}

void
printFileHeader(std::ostream &os, BuildHistory *bh, const Build &build,
                const File &file)
{
    std::vector<std::string> v = describeFile(bh, build, file, !DoSpacing{});
    os << Label{"File"} << ": " << v[0] << ", "
       << v[1] << '(' << v[2] << "), "
       << v[3] << '(' << v[4] << ")\n";
}

std::vector<std::string>
describeFile(BuildHistory *bh, const Build &build, const File &file,
             DoSpacing spacing)
{
    const std::string sep = spacing ? " / " : "/";
    CovInfo covInfo(file);
    CovChange covChange = getFileCovChange(bh, build, file.getPath(), nullptr,
                                           covInfo);

    return {
        file.getPath(),
        covInfo.formatCoverageRate(),
        covInfo.formatLines(sep),
        covChange.formatCoverageRate(),
        covChange.formatLines(sep),
    };
}

void
printFileHeader(std::ostream &os, BuildHistory *bh, const Build &build,
                const std::string &filePath, const Build *prevBuild)
{
    CovInfo covInfo;
    if (boost::optional<File &> file = build.getFile(filePath)) {
        covInfo = CovInfo(*file);
    }

    CovChange covChange = getFileCovChange(bh, build, filePath, nullptr,
                                           covInfo, prevBuild);
    printFileHeader(os, filePath, covInfo, covChange);
}

static void
printFileHeader(std::ostream &os, const std::string &filePath,
                const CovInfo &covInfo, const CovChange &covChange)
{
    os << Label{"File"} << ": " << filePath << ", "
       << covInfo.formatCoverageRate() << ' '
       << '(' << covInfo.formatLines("/") << "), "
       << covChange.formatCoverageRate() << ' '
       << '(' << covChange.formatLines("/") << ")\n";
}

static CovChange
getFileCovChange(BuildHistory *bh, const Build &build, const std::string &path,
                 boost::optional<Build> *prevBuildHint, const CovInfo &covInfo,
                 const Build *prevBuild)
{
    boost::optional<Build> prevBuildStorage;
    if (prevBuildHint == nullptr) {
        if (prevBuild != nullptr) {
            prevBuildStorage = *prevBuild;
        } else if (int prevBuildId = bh->getPreviousBuildId(build.getId())) {
            prevBuildStorage = bh->getBuild(prevBuildId);
        }
        prevBuildHint = &prevBuildStorage;
    }

    CovInfo prevCovInfo;
    if (*prevBuildHint) {
        if (boost::optional<File &> file = (*prevBuildHint)->getFile(path)) {
            prevCovInfo = CovInfo(*file);
        }
    }

    return CovChange(prevCovInfo, covInfo);
}
