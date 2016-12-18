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

#include "FilePrinter.hpp"

#include <boost/multi_array.hpp>

#include <srchilite/sourcehighlight.h>
#include <srchilite/langmap.h>

#include <cassert>

#include <algorithm>
#include <deque>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#include "decoration.hpp"

namespace {

enum class LineType
{
    Note,
    Common,
    Identical,
    Added,
    Removed
};

struct DiffLine
{
    LineType type;
    std::string text;
    int oldLine;
    int newLine;

    DiffLine(LineType type, const std::string &text,
             int oldLine, int newLine)
        : type(type), text(text), oldLine(oldLine), newLine(newLine)
    {
    }

    int getLine() const
    {
        return std::max(oldLine, newLine);
    }
};

struct local
{
    static int countWidth(int n);
    static std::deque<DiffLine> computeDiff(const std::vector<std::string> &o,
                                            const std::vector<int> &oCov,
                                            const std::vector<std::string> &n,
                                            const std::vector<int> &nCov);
};

class CoverageColumn
{
    struct Blank { const CoverageColumn &cc; };
    struct LineAt { const CoverageColumn &cc; const int lineNo; };

    friend std::ostream & operator<<(std::ostream &os, const Blank &blank)
    {
        blank.cc.printBlank();
        return os;
    }

    friend std::ostream & operator<<(std::ostream &os, const LineAt &lineAt)
    {
        lineAt.cc.printAt(lineAt.lineNo);
        return os;
    }

public:
    CoverageColumn(std::ostream &os, const std::vector<int> &coverage)
        : os(os), coverage(coverage)
    {
        const int MinHitsNumWidth = 5;
        // XXX: this could in principle be stored in database.
        const int maxHits = *std::max_element(coverage.cbegin(),
                                              coverage.cend());
        hitsNumWidth = std::max(MinHitsNumWidth, local::countWidth(maxHits));
    }

public:
    Blank blank() const
    {
        return { *this };
    }

    LineAt operator[](int lineNo) const
    {
        return { *this, lineNo };
    }

private:
    void printBlank() const
    {
        os << std::setw(hitsNumWidth) << "" << ' ';
    }

    void printAt(std::size_t lineNo) const
    {
        if (lineNo >= coverage.size()) {
            os << (decor::red_bg + decor::inv + decor::bold
               << std::setw(hitsNumWidth) << "ERROR" << ' ');
            return;
        }

        const int hits = coverage[lineNo];
        decor::Decoration dec;
        std::string prefix;
        if (hits == 0) {
            dec = decor::red_fg + decor::inv + decor::bold;
            prefix = "x0";
        } else if (hits > 0) {
            dec = decor::green_fg + decor::inv + decor::bold;
            prefix = 'x' + std::to_string(hits);
        }

        os << (dec << std::setw(hitsNumWidth) << prefix << ' ');
    }

private:
    std::ostream &os;
    const std::vector<int> &coverage;
    int hitsNumWidth;
};

}

Text::Text(const std::string &text) : iss(text)
{
}

const std::vector<std::string> &
Text::asLines()
{
    if (lines.empty()) {
        const std::string &text = iss.str();
        lines.reserve(std::count(text.cbegin(), text.cend(), '\n') + 1);
        for (std::string line; std::getline(iss, line); ) {
            lines.push_back(line);
        }
    }
    return lines;
}

std::istringstream &
Text::asStream()
{
    iss.clear();
    iss.seekg(0);
    return iss;
}

std::size_t
Text::size()
{
    return asLines().size();
}

FilePrinter::FilePrinter() : sourceHighlight("esc256.outlang"),
                             langMap("lang.map")
{
    sourceHighlight.setStyleFile("esc256.style");
    // XXX: hard-coded value of 4 spaces per tabulation.
    sourceHighlight.setTabSpaces(4);
}

void
FilePrinter::print(std::ostream &os, const std::string &path,
                   const std::string &contents,
                   const std::vector<int> &coverage)
{
    const int MinLineNoWidth = 5;

    std::istringstream iss(contents);
    std::stringstream ss;
    sourceHighlight.highlight(iss, ss, getLang(path));

    const int nLines = coverage.size();
    const int lineNoWidth = std::max(MinLineNoWidth, local::countWidth(nLines));

    CoverageColumn covCol(os, coverage);

    std::size_t lineNo = 0U;
    for (std::string fileLine; std::getline(ss, fileLine); ++lineNo) {
        os << (decor::white_bg + decor::black_fg
           << std::setw(lineNoWidth) << lineNo + 1 << ' ')
           << covCol[lineNo] << ": " << fileLine << '\n';
    }

    if (lineNo < coverage.size()) {
        os << (decor::red_bg + decor::bold
           << "ERROR:") << " not enough lines in the file.\n";
    } else if (lineNo > coverage.size()) {
        os << (decor::red_bg + decor::bold
           << "ERROR:") << " too many lines in the file.\n";
    }
}

int
local::countWidth(int n)
{
    int width = 1;
    while (n > 0) {
        n /= 10;
        ++width;
    }
    return width;
}

void
FilePrinter::printDiff(std::ostream &os, const std::string &path,
                       Text &oText, const std::vector<int> &oCov,
                       Text &nText, const std::vector<int> &nCov)
{
    const std::vector<std::string> &o = oText.asLines();
    const std::vector<std::string> &n = nText.asLines();

    assert(o.size() == oCov.size() && n.size() == nCov.size() &&
           "Coverage information must be accurate");

    const std::deque<DiffLine> diff = local::computeDiff(o, oCov, n, nCov);
    CoverageColumn oldCovCol(os, oCov), newCovCol(os, nCov);

    const std::string &lang = getLang(path);
    std::stringstream fss, sss;
    sourceHighlight.highlight(oText.asStream(), fss, lang);
    sourceHighlight.highlight(nText.asStream(), sss, lang);

    auto getOLine = [&fss, currLine = -1](int lineNo) mutable {
        std::string line;
        do {
            std::getline(fss, line);
        } while (++currLine != lineNo);
        return line;
    };
    auto getNLine = [&sss, currLine = -1](int lineNo) mutable {
        std::string line;
        do {
            std::getline(sss, line);
        } while (++currLine != lineNo);
        return line;
    };

    decor::Decoration additionDec = decor::green_fg + decor::bold;
    decor::Decoration removalDec = decor::red_fg + decor::bold;
    for (const DiffLine &line : diff) {
        switch (line.type) {
            case LineType::Added:
                os << oldCovCol.blank() << ':'
                   << newCovCol[line.newLine] << ':'
                   << (additionDec << '+')
                   << getNLine(line.newLine);
                break;
            case LineType::Removed:
                os << oldCovCol[line.oldLine] << ':'
                   << newCovCol.blank() << ':'
                   << (removalDec << '-')
                   << getOLine(line.oldLine);
                break;
            case LineType::Note:
                os << " <<< " + line.text + " >>>";
                break;
            case LineType::Common:
                os << oldCovCol[line.oldLine] << ':'
                   << newCovCol[line.newLine] << ": "
                   << getOLine(line.oldLine);
                break;
            case LineType::Identical:
                os << oldCovCol.blank() << ':'
                   << newCovCol.blank() << ": "
                   << getOLine(line.oldLine);
                break;
        }
        os << '\n';
    }
}

std::deque<DiffLine>
local::computeDiff(const std::vector<std::string> &o,
                   const std::vector<int> &oCov,
                   const std::vector<std::string> &n,
                   const std::vector<int> &nCov)
{
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

    std::deque<DiffLine> diff;
    int identicalLines = 0;

    auto foldIdentical = [&identicalLines, &diff]() {
        if (identicalLines > 4) {
            diff.erase(diff.cbegin() + 1,
                       diff.cbegin() + (identicalLines - 1));
            diff.emplace(diff.cbegin() + 1,
                         LineType::Note,
                         std::to_string(identicalLines - 2) +
                         " identical lines folded", -1, -1);
        }
        identicalLines = 0;
    };

    // Compose results with folding of long runs of identical lines (longer
    // than two lines).
    int i = o.size(), j = n.size();
    while (i != 0 || j != 0) {
        if (i == 0) {
            --j;
            foldIdentical();
            diff.emplace_front(LineType::Added, n[j], -1, j);
        } else if (j == 0) {
            foldIdentical();
            --i;
            diff.emplace_front(LineType::Removed, o[i], i, -1);
        } else if (d[i][j] == d[i][j - 1] + 1) {
            foldIdentical();
            --j;
            diff.emplace_front(LineType::Added, n[j], -1, j);
        } else if (d[i][j] == d[i - 1][j] + 1) {
            foldIdentical();
            --i;
            diff.emplace_front(LineType::Removed, o[i], i, -1);
        } else if (o[--i] == n[--j]) {
            if (oCov[i] == -1 && nCov[j] == -1) {
                diff.emplace_front(LineType::Identical, o[i], i, j);
                ++identicalLines;
            } else {
                foldIdentical();
                diff.emplace_front(LineType::Common, o[i], i, j);
            }
        }
    }
    foldIdentical();

    return diff;
}

std::string
FilePrinter::getLang(const std::string &path)
{
    std::string lang = langMap.getMappedFileNameFromFileName(path);
    if (lang.empty()) {
        lang = "cpp.lang";
    }
    return lang;
}
