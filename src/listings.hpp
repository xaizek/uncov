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

#ifndef UNCOV__LISTINGS_HPP__
#define UNCOV__LISTINGS_HPP__

#include <iosfwd>
#include <string>
#include <vector>

class Build;
class BuildHistory;
class File;

std::vector<std::string> describeBuild(BuildHistory *bh, const Build &build,
                                       bool extraAlign);

/**
 * @brief Formats information about directories within the build as a table.
 *
 * Returned table consists of the following columns:
 *
 *  * Directory
 *  * Coverage
 *  * C/R Lines
 *  * Cov Change
 *  * C/M/R Line Changes
 *
 * @param bh Object maintaining history of all builds.
 * @param build The build we're describing.
 * @param dirFilter Root directory that filters displayed dirs.  Can be empty.
 *
 * @returns Table of strings describing the build.
 */
std::vector<std::vector<std::string>>
describeBuildDirs(BuildHistory *bh, const Build &build,
                  const std::string &dirFilter);

/**
 * @brief Formats information about files within the build as a table.
 *
 * Returned table consists of the following columns:
 *
 *  * File
 *  * Coverage
 *  * C/R Lines
 *  * Cov Change
 *  * C/M/R Line Changes
 *
 * @param bh Object maintaining history of all builds.
 * @param build The build we're describing.
 * @param dirFilter Root directory that filters displayed files.  Can be empty.
 * @param changedOnly Filter out all files which unchanged coverage.
 *
 * @returns Table of strings describing the build.
 */
std::vector<std::vector<std::string>>
describeBuildFiles(BuildHistory *bh, const Build &build,
                   const std::string &dirFilter, bool changedOnly);

void printBuildHeader(std::ostream &os, BuildHistory *bh, const Build &build);

void printFileHeader(std::ostream &os, BuildHistory *bh, const Build &build,
                     const File &file);

void printFileHeader(std::ostream &os, BuildHistory *bh, const Build &build,
                     const std::string &filePath);

#endif // UNCOV__LISTINGS_HPP__
