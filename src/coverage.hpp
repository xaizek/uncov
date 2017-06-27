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

#ifndef UNCOV__COVERAGE_HPP__
#define UNCOV__COVERAGE_HPP__

#include <string>

/**
 * @file coverage.hpp
 *
 * Coverage computation and formatting.
 */

/**
 * @brief Computes and formats coverage conformation.
 */
class CovInfo
{
    //! To give access to @c coveredCount and @c missedCount.
    friend class CovChange;

public:
    /**
     * @brief Constructs empty coverage information.
     */
    CovInfo() = default;
    /**
     * @brief Constructs coverage information from a Coverable.
     *
     * Coverable is any class that implements @c getCoveredCount() and
     * @c getMissedCount() functions that return @c int.
     *
     * @tparam T Type of the Coverable.
     *
     * @param coverable Source of the coverage information.
     */
    template <typename T>
    explicit CovInfo(const T &coverable)
        : coveredCount(coverable.getCoveredCount()),
          missedCount(coverable.getMissedCount())
    {
    }

public:
    /**
     * @brief Adds coverage information.
     *
     * @param rhs Another coverage information.
     */
    void add(const CovInfo &rhs);
    /**
     * @brief Formats coverage rate in the new state as a string.
     *
     * @returns Formatted string.
     */
    std::string formatCoverageRate() const;
    /**
     * @brief Formats coverage statistics in lines.
     *
     * Format is covered/missed/relevant.
     *
     * @param separator Separator of statistics.
     *
     * @returns Formatted string.
     */
    std::string formatLines(const std::string &separator) const;

private:
    /**
     * @brief Retrieves rate of source coverage.
     *
     * @returns The rate.
     */
    float getCoverage() const;
    /**
     * @brief Retrieves number of lines that are relevant for code coverage.
     *
     * @returns The number.
     */
    int getRelevantLines() const;

private:
    int coveredCount = 0; //!< Number of covered lines.
    int missedCount = 0;  //!< Number of missed lines.
};

/**
 * @brief Computes and formats coverage change.
 */
class CovChange
{
public:
    /**
     * @brief Computes coverage change between two states.
     *
     * @param oldCov Old coverage information.
     * @param newCov New coverage information.
     */
    CovChange(const CovInfo &oldCov, const CovInfo &newCov);

public:
    /**
     * @brief Whether new coverage information differs from the old one.
     *
     * @returns @c true if so, @c false otherwise.
     */
    bool isChanged() const;
    /**
     * @brief Formats change of coverage rate in the new state as a string.
     *
     * @returns Formatted string.
     */
    std::string formatCoverageRate() const;
    /**
     * @brief Formats changes coverage statistics in lines.
     *
     * Format is covered/missed/relevant.
     *
     * @param separator Separator of statistics.
     * @param width     Minimal width for first and second statistics.
     *
     * @returns Formatted string.
     */
    std::string formatLines(const std::string &separator, int width = 0) const;

private:
    float coverageChange; //!< Change of covered lines in percents.
    int coveredChange;    //!< Change of covered lines in lines.
    int missedChange;     //!< Change of missed lines in lines.
    int relevantChange;   //!< Change of relevant lines in lines.
};

#endif // UNCOV__COVERAGE_HPP__
