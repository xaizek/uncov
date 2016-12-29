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

class CovInfo
{
    friend class CovChange;

public:
    CovInfo() = default;
    template <typename T>
    explicit CovInfo(const T &coverable)
        : coveredCount(coverable.getCoveredCount()),
          missedCount(coverable.getMissedCount())
    {
    }

public:
    void add(const CovInfo &rhs);
    std::string formatCoverageRate() const;
    std::string formatLines(const std::string &separator) const;

private:
    float getCoverage() const;
    int getRelevantLines() const;

private:
    int coveredCount = 0;
    int missedCount = 0;
};

class CovChange
{
public:
    CovChange(const CovInfo &oldCov, const CovInfo &newCov);

public:
    bool isChanged() const;
    std::string formatCoverageRate() const;
    std::string formatLines(const std::string &separator) const;

private:
    float coverageChange;
    int coveredChange;
    int missedChange;
    int relevantChange;
};

#endif // UNCOV__COVERAGE_HPP__
