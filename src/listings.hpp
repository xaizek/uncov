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

#ifndef UNCOV_LISTINGS_HPP_
#define UNCOV_LISTINGS_HPP_

#include <iosfwd>
#include <string>
#include <vector>

#include "utils/Flag.hpp"

/**
 * @file listings.hpp
 *
 * @brief This unit provides functions for listings all sorts of things.
 */

class Build;
class BuildHistory;
class File;

//! Boolean flag type for controlling extra alignment.
using DoExtraAlign = Flag<struct DoExtraAlignTag>;

//! Boolean flag type for controlling spacing.
using DoSpacing = Flag<struct DoSpacingTag>;

//! Boolean flag type for limiting list of files to changed only.
using ListChangedOnly = Flag<struct ListChangedOnlyTag>;

//! Boolean flag type for limiting list of files to direct children only.
using ListDirectOnly = Flag<struct ListDirectOnlyTag>;

/**
 * @brief Formats information about specified build as a table row.
 *
 * Returned row consists of the following columns:
 *
 *  * Build Id
 *  * Coverage
 *  * C/R Lines
 *  * Cov Change
 *  * C/M/R Line Changes
 *  * Ref
 *  * Commit
 *  * Time
 *
 * @param bh Object maintaining history of all builds.
 * @param build The build we're describing.
 * @param extraAlign Whether alignment should look nice in a table.
 * @param spacing Whether to add extra spacing around `/` separator.
 * @param prevBuild Build to compute coverage change against.
 *
 * @returns Row with information about the build.
 */
std::vector<std::string> describeBuild(BuildHistory *bh, const Build &build,
                                       DoExtraAlign extraAlign,
                                       DoSpacing spacing,
                                       const Build *prevBuild = nullptr);

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
 * @param prevBuild Build to compute coverage change against.
 *
 * @returns Table of strings describing the build.
 */
std::vector<std::vector<std::string>>
describeBuildDirs(BuildHistory *bh, const Build &build,
                  const std::string &dirFilter,
                  const Build *prevBuild = nullptr);

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
 * @param directOnly Print only direct descendants of the directory.
 * @param prevBuild Build to compute coverage change against.
 *
 * @returns Table of strings describing the build.
 */
std::vector<std::vector<std::string>>
describeBuildFiles(BuildHistory *bh, const Build &build,
                   const std::string &dirFilter, ListChangedOnly changedOnly,
                   ListDirectOnly directOnly, const Build *prevBuild = nullptr);

/**
 * @brief Prints build header which is a one line description of it.
 *
 * @param os Stream to print the information onto.
 * @param bh Object maintaining history of all builds.
 * @param build The build we're describing.
 * @param prevBuild Build to compute coverage change against.
 */
void printBuildHeader(std::ostream &os, BuildHistory *bh, const Build &build,
                      const Build *prevBuild = nullptr);

/**
 * @brief Prints file header which is a one line description of it.
 *
 * @param os Stream to print the information onto.
 * @param bh Object maintaining history of all builds.
 * @param build Build of the file.
 * @param file File we're describing.
 */
void printFileHeader(std::ostream &os, BuildHistory *bh, const Build &build,
                     const File &file);

/**
 * @brief Formats information about specified build as a table row.
 *
 * Returned row consists of the following columns:
 *
 *  * File path
 *  * Coverage
 *  * C/R Lines
 *  * Cov Change
 *  * C/M/R Line Changes
 *
 * @param bh Object maintaining history of all builds.
 * @param build Build we're interested in.
 * @param file File to describe.
 * @param spacing Whether to add extra spacing around `/` separator.
 *
 * @returns Row with information about the build.
 */
std::vector<std::string> describeFile(BuildHistory *bh, const Build &build,
                                      const File &file, DoSpacing spacing);

/**
 * @brief Prints file header which is a one line description of it.
 *
 * @param os Stream to print the information onto.
 * @param bh Object maintaining history of all builds.
 * @param build Build of the file.
 * @param filePath Path of file to describe.
 * @param prevBuild Build to compute coverage change against.
 */
void printFileHeader(std::ostream &os, BuildHistory *bh, const Build &build,
                     const std::string &filePath,
                     const Build *prevBuild = nullptr);

#endif // UNCOV_LISTINGS_HPP_
