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

#include <srchilite/sourcehighlight.h>
#include <srchilite/langmap.h>
#include <srchilite/lineranges.h>

#include <algorithm>
#include <deque>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#include "FileComparator.hpp"
#include "integration.hpp"
#include "printing.hpp"

namespace {

int
countWidth(int n)
{
    int width = 1;
    while (n > 0) {
        n /= 10;
        ++width;
    }
    return width;
}

class CoverageColumn
{
    struct Blank { const CoverageColumn &cc; };
    struct LineAt { bool active; const CoverageColumn &cc; const int lineNo; };

    friend std::ostream & operator<<(std::ostream &os, const Blank &blank)
    {
        blank.cc.printBlank();
        return os;
    }

    friend std::ostream & operator<<(std::ostream &os, const LineAt &lineAt)
    {
        lineAt.cc.printAt(lineAt.lineNo, lineAt.active);
        return os;
    }

public:
    CoverageColumn(std::ostream &os, const std::vector<int> &coverage)
        : os(os), coverage(coverage)
    {
        const int MinHitsNumWidth = 5;
        // XXX: this could in principle be stored in database.
        int maxHits = 0;
        if (!coverage.empty()) {
            maxHits = *std::max_element(coverage.cbegin(), coverage.cend());
        }
        hitsNumWidth = std::max(MinHitsNumWidth, countWidth(maxHits));
    }

public:
    Blank blank() const
    {
        return { *this };
    }

    LineAt active(int lineNo) const
    {
        return { true, *this, lineNo };
    }

    LineAt inactive(int lineNo) const
    {
        return { false, *this, lineNo };
    }

private:
    void printBlank() const
    {
        os << std::setw(hitsNumWidth) << "" << ' ';
    }

    void printAt(std::size_t lineNo, bool active) const
    {
        if (lineNo >= coverage.size()) {
            os << std::setw(hitsNumWidth) << ErrorMsg{"ERROR "};
            return;
        }

        os << std::setw(hitsNumWidth);
        if (active) {
            os << HitsCount{coverage[lineNo]};
        } else {
            os << SilentHitsCount{coverage[lineNo]};
        }
    }

private:
    std::ostream &os;
    const std::vector<int> &coverage;
    int hitsNumWidth;
};

}

FilePrinter::FilePrinter(bool allowColors)
    : colorizeOutput(allowColors && isOutputToTerminal()),
      sourceHighlight("esc256.outlang"),
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
    std::stringstream ss = highlight(iss, getLang(path));

    const int nLines = coverage.size();
    const int lineNoWidth = std::max(MinLineNoWidth, countWidth(nLines));

    CoverageColumn covCol(os, coverage);

    std::size_t lineNo = 0U;
    for (std::string fileLine; std::getline(ss, fileLine); ++lineNo) {
        os << std::setw(lineNoWidth) << LineNo{lineNo + 1}
           << covCol.active(lineNo) << ": " << fileLine << '\n';
    }

    if (lineNo < coverage.size()) {
        os << ErrorMsg{"ERROR"} << ": not enough lines in the file.\n";
    } else if (lineNo > coverage.size()) {
        os << ErrorMsg{"ERROR"} << ": too many lines in the file.\n";
    }
}

void
FilePrinter::printDiff(std::ostream &os, const std::string &path,
                       std::istream &oText, const std::vector<int> &oCov,
                       std::istream &nText, const std::vector<int> &nCov,
                       const FileComparator &comparator)
{
    const std::deque<DiffLine> &diff = comparator.getDiffSequence();

    srchilite::LineRanges fLines, sLines;
    for (const DiffLine &line : diff) {
        switch (line.type) {
            case DiffLineType::Added:
                sLines.addRange(std::to_string(line.newLine + 1));
                break;
            case DiffLineType::Removed:
            case DiffLineType::Common:
            case DiffLineType::Identical:
                fLines.addRange(std::to_string(line.oldLine + 1));
                break;
            case DiffLineType::Note:
                // Do nothing.
                break;
        }
    }

    const std::string &lang = getLang(path);
    std::stringstream fss = highlight(oText, lang, &fLines);
    std::stringstream sss = highlight(nText, lang, &sLines);

    auto getLine = [](std::stringstream &ss) {
        std::string line;
        std::getline(ss, line);
        return line;
    };

    CoverageColumn oldCovCol(os, oCov), newCovCol(os, nCov);
    for (const DiffLine &line : diff) {
        switch (line.type) {
            case DiffLineType::Added:
                os << oldCovCol.blank() << ':'
                   << newCovCol.active(line.newLine) << ':'
                   << LineAdded{getLine(sss)};
                break;
            case DiffLineType::Removed:
                os << oldCovCol.active(line.oldLine) << ':'
                   << newCovCol.blank() << ':'
                   << LineRemoved{getLine(fss)};
                break;
            case DiffLineType::Note:
                os << " <<< " + line.text + " >>>";
                break;
            case DiffLineType::Common:
                os << oldCovCol.active(line.oldLine) << ':'
                   << newCovCol.active(line.newLine) << ": "
                   << getLine(fss);
                break;
            case DiffLineType::Identical:
                os << oldCovCol.inactive(line.oldLine) << ':'
                   << newCovCol.inactive(line.newLine) << ": "
                   << getLine(fss);
                break;
        }
        os << '\n';
    }
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

std::stringstream
FilePrinter::highlight(std::istream &text, const std::string &lang,
                       srchilite::LineRanges *ranges)
{
    std::stringstream ss;

    if (colorizeOutput) {
        sourceHighlight.setLineRanges(ranges);
        sourceHighlight.highlight(text, ss, lang);
    } else {
        ss << text.rdbuf();
    }

    return ss;
}
