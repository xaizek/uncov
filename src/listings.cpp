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

#include "BuildHistory.hpp"
#include "coverage.hpp"
#include "decoration.hpp"

static std::map<std::string, CovInfo> getDirsCoverage(const Build &build);
static CovChange getBuildCovChange(BuildHistory *bh, const Build &build,
                                   const CovInfo &covInfo);
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
        build.getRef()
    };
}

std::vector<std::vector<std::string>>
describeBuildDirs(BuildHistory *bh, const Build &build)
{
    std::map<std::string, CovInfo> newDirs = getDirsCoverage(build);

    std::map<std::string, CovInfo> prevDirs;
    if (const int prevBuildId = bh->getPreviousBuildId(build.getId())) {
        prevDirs = getDirsCoverage(*bh->getBuild(prevBuildId));
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
getDirsCoverage(const Build &build)
{
    std::map<std::string, CovInfo> dirs;
    for (const std::string &filePath : build.getPaths()) {
        boost::filesystem::path dirPath = filePath;
        dirPath.remove_filename();

        File &file = *build.getFile(filePath);
        CovInfo &info = dirs[dirPath.string()];

        info.add(CovInfo(file));
    }
    return dirs;
}

std::vector<std::vector<std::string>>
describeBuildFiles(BuildHistory *bh, const Build &build)
{
    const std::vector<std::string> &paths = build.getPaths();

    std::vector<std::vector<std::string>> rows;
    rows.reserve(paths.size());

    boost::optional<Build> prevBuild;
    if (const int prevBuildId = bh->getPreviousBuildId(build.getId())) {
        prevBuild = bh->getBuild(prevBuildId);
    }

    for (const std::string &filePath : paths) {
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

    os << (decor::bold << "Build:") << " #" << build.getId() << ", "
       << covInfo.formatCoverageRate() << ' '
       << '(' << covInfo.formatLines("/") << "), "
       << covChange.formatCoverageRate() << ' '
       << '(' << covChange.formatLines("/") << "), "
       << build.getRefName() << " at " << build.getRef() << '\n';
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

    os << (decor::bold << "File: ") << file.getPath() << ", "
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