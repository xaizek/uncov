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

#include "coverage.hpp"

#include <iomanip>
#include <sstream>
#include <string>

void
CovInfo::add(const CovInfo &rhs)
{
    coveredCount += rhs.coveredCount;
    uncoveredCount += rhs.uncoveredCount;
}

std::string
CovInfo::formatCoverageRate() const
{
    if (getRelevantLines() == 0) {
        return "100";
    }

    std::ostringstream oss;
    oss << std::fixed << std::right
        << std::setprecision(2) << getCoverage();
    return oss.str();
}

std::string
CovInfo::formatLines(const std::string &separator) const
{
    return std::to_string(coveredCount) + separator
         + std::to_string(getRelevantLines());
}

float
CovInfo::getCoverage() const
{
    if (getRelevantLines() == 0) {
        // Return 0 instead of NaN here to make it easier for CovChange.
        return 0.0f;
    }
    return 100*coveredCount/static_cast<float>(getRelevantLines());
}

int
CovInfo::getRelevantLines() const
{
    return coveredCount + uncoveredCount;
}

CovChange::CovChange(const CovInfo &oldCov, const CovInfo &newCov)
{
    coverageChange = newCov.getCoverage() - oldCov.getCoverage();
    coveredChange = newCov.coveredCount - oldCov.coveredCount;
    uncoveredChange = newCov.uncoveredCount - oldCov.uncoveredCount;
    relevantChange = newCov.getRelevantLines() - oldCov.getRelevantLines();
}

std::string
CovChange::formatCoverageRate() const
{
    std::ostringstream oss;
    oss << std::fixed << std::right << std::showpos
        << std::setprecision(4) << coverageChange;
    return oss.str();
}

std::string
CovChange::formatLines(const std::string &separator) const
{
    std::ostringstream oss;
    oss << std::showpos
        << coveredChange << separator
        << uncoveredChange << separator << relevantChange;
    return oss.str();
}