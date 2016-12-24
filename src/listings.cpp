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
                                   const CovInfo &covInfo);
static void printFileHeader(std::ostream &os, const std::string &filePath,
                            const CovInfo &covInfo, const CovChange &covChange);
static CovChange getFileCovChange(BuildHistory *bh, const Build &build,
                                  const std::string &path,
                                  boost::optional<Build> *prevBuildHint,
                                  const CovInfo &covInfo);

std::vector<std::string>
describeBuild(BuildHistory *bh, const Build &build)
{
    CovInfo covInfo(build);
    CovChange covChange = getBuildCovChange(bh, build, covInfo);

    return {
        "#" + std::to_string(build.getId()),
        covInfo.formatCoverageRate(),
        covInfo.formatLines(" / "),
        covChange.formatCoverageRate(),
        covChange.formatLines(" / "),
        build.getRefName(),
        Revision{build.getRef()},
        Time{build.getTimestamp()}
    };
}

std::vector<std::vector<std::string>>
describeBuildDirs(BuildHistory *bh, const Build &build,
                  const std::string &dirFilter)
{
    std::map<std::string, CovInfo> newDirs = getDirsCoverage(build, dirFilter);

    std::map<std::string, CovInfo> prevDirs;
    if (const int prevBuildId = bh->getPreviousBuildId(build.getId())) {
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
            covChange.formatLines(" / ")
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
                   const std::string &dirFilter)
{
    const std::vector<std::string> &paths = build.getPaths();

    std::vector<std::vector<std::string>> rows;
    rows.reserve(paths.size());

    boost::optional<Build> prevBuild;
    if (const int prevBuildId = bh->getPreviousBuildId(build.getId())) {
        prevBuild = bh->getBuild(prevBuildId);
    }

    for (const std::string &filePath : paths) {
        if (!pathIsInSubtree(dirFilter, filePath)) {
            continue;
        }

        CovInfo covInfo(*build.getFile(filePath));
        CovChange covChange = getFileCovChange(bh, build, filePath,
                                               &prevBuild, covInfo);

        rows.push_back({
            filePath,
            covInfo.formatCoverageRate(),
            covInfo.formatLines(" / "),
            covChange.formatCoverageRate(),
            covChange.formatLines(" / "),
        });
    }

    return rows;
}

void
printBuildHeader(std::ostream &os, BuildHistory *bh, const Build &build)
{
    CovInfo covInfo(build);
    CovChange covChange = getBuildCovChange(bh, build, covInfo);

    os << Label{"Build"} << ": #" << build.getId() << ", "
       << covInfo.formatCoverageRate() << ' '
       << '(' << covInfo.formatLines("/") << "), "
       << covChange.formatCoverageRate() << ' '
       << '(' << covChange.formatLines("/") << "), "
       << build.getRefName() << " at " << Revision{build.getRef()} << '\n';
}

static CovChange
getBuildCovChange(BuildHistory *bh, const Build &build, const CovInfo &covInfo)
{
    CovInfo prevCovInfo;
    if (const int prevBuildId = bh->getPreviousBuildId(build.getId())) {
        prevCovInfo = CovInfo(*bh->getBuild(prevBuildId));
    }

    return CovChange(prevCovInfo, covInfo);
}

void
printFileHeader(std::ostream &os, BuildHistory *bh, const Build &build,
                const File &file)
{
    CovInfo covInfo(file);
    CovChange covChange = getFileCovChange(bh, build, file.getPath(), nullptr,
                                           covInfo);
    printFileHeader(os, file.getPath(), covInfo, covChange);
}

void
printFileHeader(std::ostream &os, BuildHistory *bh, const Build &build,
                const std::string &filePath)
{
    CovInfo covInfo;
    CovChange covChange = getFileCovChange(bh, build, filePath, nullptr,
                                           covInfo);
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
                 boost::optional<Build> *prevBuildHint, const CovInfo &covInfo)
{
    boost::optional<Build> prevBuild;
    if (prevBuildHint == nullptr) {
        if (const int prevBuildId = bh->getPreviousBuildId(build.getId())) {
            prevBuild = bh->getBuild(prevBuildId);
        }
        prevBuildHint = &prevBuild;
    }

    CovInfo prevCovInfo;
    if (*prevBuildHint) {
        boost::optional<File &> file = (*prevBuildHint)->getFile(path);
        if (file) {
            prevCovInfo = CovInfo(*file);
        }
    }

    return CovChange(prevCovInfo, covInfo);
}
