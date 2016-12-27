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

#include <deque>
#include <string>
#include <vector>

static bool validate(const std::vector<std::string> &o,
                     const std::vector<int> &oCov,
                     const std::vector<std::string> &n,
                     const std::vector<int> &nCov,
                     std::string &error);
static inline int sign(int i);

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

    using size_type = std::vector<std::string>::size_type;

    // Narrow portion of lines that should be compared by throwing away matching
    // leading and trailing lines.
    size_type ol = 0U, nl = 0U, ou = o.size(), nu = n.size();
    while (ol < ou && nl < nu && o[ol] == n[nl]) {
        ++ol;
        ++nl;
    }
    while (ou > ol && nu > nl && o[ou - 1U] == n[nu - 1U]) {
        --ou;
        --nu;
    }

    boost::multi_array<int, 2> d(boost::extents[ou - ol + 1U][nu - nl + 1U]);

    // Modified edit distance finding.
    for (size_type i = 0U; i <= ou - ol; ++i) {
        for (size_type j = 0U; j <= nu - nl; ++j) {
            if (i == 0U) {
                d[i][j] = j;
            } else if (j == 0U) {
                d[i][j] = i;
            } else {
                d[i][j] = std::min(d[i - 1U][j] + 1, d[i][j - 1U] + 1);
                if (o[ol + i - 1U] == n[nl + j - 1U]) {
                    d[i][j] = std::min(d[i - 1U][j - 1U], d[i][j]);
                }
            }
        }
    }

    size_type identicalLines = 0U;

    auto foldIdentical = [this, &identicalLines](bool last) {
        if (identicalLines > 4) {
            int startContext = last ? 0 : 1;
            int endContext = identicalLines == diffSeq.size() ? 0 : 1;
            int context = startContext + endContext;

            diffSeq.erase(diffSeq.cbegin() + startContext,
                          diffSeq.cbegin() + (identicalLines - endContext));
            diffSeq.emplace(diffSeq.cbegin() + startContext, DiffLineType::Note,
                            std::to_string(identicalLines - context) +
                            " lines folded", -1, -1);
        }
        identicalLines = 0U;
    };

    auto maybeConsiderIdentical = [&identicalLines, &foldIdentical](int hits) {
        if (hits == -1) {
            ++identicalLines;
        } else {
            foldIdentical(false);
        }
    };

    auto handleSameLines = [&](size_type i, size_type j) {
        const int oHits = considerHits ? oCov[i] : sign(oCov[i]);
        const int nHits = considerHits ? nCov[j] : sign(nCov[j]);
        if (oHits == nHits) {
            diffSeq.emplace_front(DiffLineType::Identical, o[i], i, j);
            ++identicalLines;
        } else {
            foldIdentical(false);
            diffSeq.emplace_front(DiffLineType::Common, o[i], i, j);
        }
    };

    // Compose results with folding of long runs of identical lines (longer
    // than two lines).  Mind that we go from last to first element and loops
    // below process tail, middle and then head parts of the files.

    for (size_type k = o.size(), l = n.size(); k > ou; --k, --l) {
        handleSameLines(k - 1U, l - 1U);
    }

    int i = ou - ol, j = nu - nl;
    while (i != 0U || j != 0U) {
        if (i == 0) {
            maybeConsiderIdentical(nCov[nl + --j]);
            diffSeq.emplace_front(DiffLineType::Added, n[nl + j], -1, nl + j);
        } else if (j == 0) {
            maybeConsiderIdentical(oCov[ol + --i]);
            diffSeq.emplace_front(DiffLineType::Removed, o[ol + i], ol + i, -1);
        } else if (d[i][j] == d[i][j - 1] + 1) {
            maybeConsiderIdentical(nCov[nl + --j]);
            diffSeq.emplace_front(DiffLineType::Added, n[nl + j], -1, nl + j);
        } else if (d[i][j] == d[i - 1][j] + 1) {
            maybeConsiderIdentical(oCov[ol + --i]);
            diffSeq.emplace_front(DiffLineType::Removed, o[ol + i], ol + i, -1);
        } else if (o[ol + --i] == n[nl + --j]) {
            handleSameLines(ol + i, nl + j);
        }
    }

    for (size_type i = ol; i != 0U; --i) {
        handleSameLines(i - 1U, i - 1U);
    }

    equal = (identicalLines == diffSeq.size());

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

static inline int
sign(int i)
{
    if (i < 0) {
        return -1;
    } else if (i == 0) {
        return 0;
    } else {
        return +1;
    }
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
    return diffSeq;
}
