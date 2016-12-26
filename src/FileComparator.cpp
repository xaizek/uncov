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

#include "FileComparator.hpp"

#include <boost/multi_array.hpp>

#include <cstddef>

#include <deque>
#include <string>
#include <vector>

static bool validate(const std::vector<std::string> &o,
                     const std::vector<int> &oCov,
                     const std::vector<std::string> &n,
                     const std::vector<int> &nCov,
                     std::string &error);

FileComparator::FileComparator(const std::vector<std::string> &o,
                               const std::vector<int> &oCov,
                               const std::vector<std::string> &n,
                               const std::vector<int> &nCov,
                               bool considerHits)
{
    valid = validate(o, oCov, n, nCov, inputError);
    if (!valid) {
        equal = false;
        return;
    }

    boost::multi_array<int, 2> d(boost::extents[o.size() + 1U][n.size() + 1U]);

    // Modified edit distance finding.
    using size_type = std::vector<std::string>::size_type;
    for (size_type i = 0U, nf = o.size(); i <= nf; ++i) {
        for (size_type j = 0U, ns = n.size(); j <= ns; ++j) {
            if (i == 0U) {
                d[i][j] = j;
            } else if (j == 0U) {
                d[i][j] = i;
            } else {
                d[i][j] = std::min(d[i - 1U][j] + 1, d[i][j - 1U] + 1);
                if (o[i - 1U] == n[j - 1U]) {
                    d[i][j] = std::min(d[i - 1U][j - 1U], d[i][j]);
                }
            }
        }
    }

    std::size_t identicalLines = 0;

    auto foldIdentical = [this, &identicalLines](bool last) {
        if (identicalLines > 4) {
            int startContext = last ? 0 : 1;
            int endContext = identicalLines == diffSequence.size() ? 0 : 1;
            int context = startContext + endContext;

            diffSequence.erase(diffSequence.cbegin() + startContext,
                               diffSequence.cbegin() +
                               (identicalLines - endContext));
            diffSequence.emplace(diffSequence.cbegin() + startContext,
                                 DiffLineType::Note,
                                 std::to_string(identicalLines - context) +
                                 " lines folded", -1, -1);
        }
        identicalLines = 0;
    };

    auto maybeConsiderIdentical = [&identicalLines, &foldIdentical](int hits) {
        if (hits == -1) {
            ++identicalLines;
        } else {
            foldIdentical(false);
        }
    };

    auto sign = [](int i) {
        if (i < 0) {
            return -1;
        } else if (i == 0) {
            return 0;
        } else {
            return +1;
        }
    };

    // Compose results with folding of long runs of identical lines (longer
    // than two lines).
    int i = o.size(), j = n.size();
    while (i != 0 || j != 0) {
        if (i == 0) {
            maybeConsiderIdentical(nCov[--j]);
            diffSequence.emplace_front(DiffLineType::Added, n[j], -1, j);
        } else if (j == 0) {
            maybeConsiderIdentical(oCov[--i]);
            diffSequence.emplace_front(DiffLineType::Removed, o[i], i, -1);
        } else if (d[i][j] == d[i][j - 1] + 1) {
            maybeConsiderIdentical(nCov[--j]);
            diffSequence.emplace_front(DiffLineType::Added, n[j], -1, j);
        } else if (d[i][j] == d[i - 1][j] + 1) {
            maybeConsiderIdentical(oCov[--i]);
            diffSequence.emplace_front(DiffLineType::Removed, o[i], i, -1);
        } else if (o[--i] == n[--j]) {
            const int oHits = considerHits ? oCov[i] : sign(oCov[i]);
            const int nHits = considerHits ? nCov[j] : sign(nCov[j]);
            if (oHits == nHits) {
                diffSequence.emplace_front(DiffLineType::Identical, o[i], i, j);
                ++identicalLines;
            } else {
                foldIdentical(false);
                diffSequence.emplace_front(DiffLineType::Common, o[i], i, j);
            }
        }
    }

    equal = (identicalLines == diffSequence.size());

    foldIdentical(true);
}

static bool
validate(const std::vector<std::string> &o, const std::vector<int> &oCov,
         const std::vector<std::string> &n, const std::vector<int> &nCov,
         std::string &error)
{
    const bool valid = (o.size() == oCov.size() && n.size() == nCov.size());
    if (o.size() != oCov.size()) {
        error += "Old state is incorrect (" + std::to_string(o.size()) +
                 " file lines vs. " + std::to_string(oCov.size()) +
                 " coverage lines)\n";
    }
    if (n.size() != nCov.size()) {
        error += "New state is incorrect (" + std::to_string(n.size()) +
                 " file lines vs. " + std::to_string(nCov.size()) +
                 " coverage lines)\n";
    }
    return valid;
}

bool
FileComparator::isValidInput() const
{
    return valid;
}

std::string
FileComparator::getInputError() const
{
    return inputError;
}

bool
FileComparator::areEqual() const
{
    return equal;
}

const std::deque<DiffLine> &
FileComparator::getDiffSequence() const
{
    return diffSequence;
}
