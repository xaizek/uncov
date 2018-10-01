// Copyright (C) 2016 xaizek <xaizek@posteo.net>
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

#include "ColorCane.hpp"
#include "FileComparator.hpp"
#include "colors.hpp"
#include "printing.hpp"

namespace {

/**
 * @brief Counts width of a number in digit places.
 *
 * @param n Number to measure.
 *
 * @returns The width plus one (for convenience) for non-zero and zero for zero.
 */
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

/**
 * @brief Auxiliary class that draws coverage column.
 */
class CoverageColumn
{
    //! Marker to print blank entry.
    struct Blank
    {
        const CoverageColumn &cc; //!< Reference to parent object.
    };
    //! Marker to print information of a specific line.
    struct LineAt
    {
        const CoverageColumn &cc; //!< Reference to parent object.
        const int lineNo;         //!< Line number.
        const bool active;        //!< Whether the entry should standout.
    };

    /**
     * @brief Redirects printing to an instance of CoverageColumn.
     *
     * @param cc    Output.
     * @param blank Marker.
     *
     * @returns @p cc.
     */
    friend ColorCane & operator<<(ColorCane &cc, const Blank &blank)
    {
        blank.cc.printBlank(cc);
        return cc;
    }

    /**
     * @brief Redirects printing to an instance of CoverageColumn.
     *
     * @param os     Output stream.
     * @param lineAt Marker.
     *
     * @returns @p os.
     */
    friend std::ostream & operator<<(std::ostream &os, const LineAt &lineAt)
    {
        lineAt.cc.printAt(os, lineAt.lineNo, lineAt.active);
        return os;
    }

    /**
     * @brief Redirects printing to an instance of CoverageColumn.
     *
     * @param cc     Output.
     * @param lineAt Marker.
     *
     * @returns @p cc.
     */
    friend ColorCane & operator<<(ColorCane &cc, const LineAt &lineAt)
    {
        lineAt.cc.printAt(cc, lineAt.lineNo, lineAt.active);
        return cc;
    }

public:
    /**
     * @brief Constructs from coverage information.
     *
     * @param coverage Coverage information.
     */
    explicit CoverageColumn(const std::vector<int> &coverage)
        : coverage(coverage)
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
    /**
     * @brief Requests marker to print blank entry.
     *
     * @returns Marker to be output to @c std::ostream.
     */
    Blank blank() const
    {
        return { *this };
    }

    /**
     * @brief Requests marker to print active number of hits.
     *
     * @param lineNo
     *
     * @returns Marker to be output to @c std::ostream.
     */
    LineAt active(int lineNo) const
    {
        return { *this, lineNo, true };
    }

    /**
     * @brief Requests marker to print inactive number of hits.
     *
     * @param lineNo
     *
     * @returns Marker to be output to @c std::ostream.
     */
    LineAt inactive(int lineNo) const
    {
        return { *this, lineNo, false };
    }

private:
    /**
     * @brief Prints blank entry.
     *
     * @param cc Output.
     */
    void printBlank(ColorCane &cc) const
    {
        cc << HitsCount{{-1, hitsNumWidth}};
    }

    /**
     * @brief Prints entry with number of hits.
     *
     * @param os     Output stream.
     * @param lineNo Line number.
     * @param active Whether number of hits should standout.
     */
    void printAt(std::ostream &os, std::size_t lineNo, bool active) const
    {
        if (lineNo >= coverage.size()) {
            os << std::setw(hitsNumWidth) << ErrorMsg{"ERROR "};
        } else if (active) {
            os << HitsCount{{coverage[lineNo], hitsNumWidth}};
        } else {
            os << SilentHitsCount{{coverage[lineNo], hitsNumWidth}};
        }
    }

    /**
     * @brief Prints entry with number of hits.
     *
     * @param cc     Output.
     * @param lineNo Line number.
     * @param active Whether number of hits should standout.
     */
    void printAt(ColorCane &cc, std::size_t lineNo, bool active) const
    {
        if (lineNo >= coverage.size()) {
            cc << ErrorMsg{"ERROR "};
        } else if (active) {
            cc << HitsCount{{coverage[lineNo], hitsNumWidth}};
        } else {
            cc << SilentHitsCount{{coverage[lineNo], hitsNumWidth}};
        }
    }

private:
    const std::vector<int> &coverage; //!< Coverage information.
    int hitsNumWidth;                 //!< Maximum width of number of hits.
};

/**
 * @brief Prints a single character into ColorCane.
 *
 * @param cc Output
 * @param c  Character to print.
 *
 * @returns @p cc
 */
ColorCane &
operator<<(ColorCane &cc, char c)
{
    cc.append(c);
    return cc;
}

}

FilePrinter::FilePrinter(const FilePrinterSettings &settings)
    : colorizeOutput(settings.isColorOutputAllowed()),
      highlighter(settings.isHtmlOutput() ? DATADIR "/srchilight/html.outlang"
                                          : "esc256.outlang"),
      langMap("lang.map")
{
    highlighter.setStyleFile(settings.isHtmlOutput() ? "default.style"
                                                     : "esc256.style");
    highlighter.setTabSpaces(settings.getTabSize());
}

void
FilePrinter::print(std::ostream &os, const std::string &path,
                   const std::string &contents,
                   const std::vector<int> &coverage, bool leaveMissedOnly)
{
    const int MinLineNoWidth = 5;

    const int nLines = coverage.size();
    const int lineNoWidth = std::max(MinLineNoWidth, countWidth(nLines));

    std::vector<int> lines;
    unsigned int uninterestingLines = 0U;
    auto foldUninteresting = [&lines, &uninterestingLines](bool last) {
        if (uninterestingLines > 4) {
            int startContext = (uninterestingLines == lines.size() ? 0 : 1);
            int endContext = last ? 0 : 1;
            int context = startContext + endContext;

            lines.erase(lines.cend() - (uninterestingLines - startContext),
                        lines.cend() - endContext);
            lines.insert(lines.cend() - endContext,
                         -(uninterestingLines - context));
        }
        uninterestingLines = 0U;
    };

    for (unsigned int i = 0U; i < coverage.size(); ++i) {
        if (leaveMissedOnly) {
            if (coverage[i] == 0) {
                foldUninteresting(false);
            } else {
                ++uninterestingLines;
            }
        }
        lines.push_back(i);
    }
    foldUninteresting(true);

    srchilite::LineRanges ranges;
    if (leaveMissedOnly) {
        std::size_t lineNo = 0U;
        for (int line : lines) {
            if (line < 0) {
                lineNo += -line;
            } else {
                ranges.addRange(std::to_string(line + 1));
                ++lineNo;
            }
        }
        ranges.addRange(std::to_string(lineNo + 1) + '-');
    }

    std::istringstream iss(contents);
    std::stringstream ss;
    highlight(ss, iss, getLang(path), leaveMissedOnly ? &ranges : nullptr);

    CoverageColumn covCol(coverage);
    std::size_t lineNo = 0U;
    std::size_t extraLines = 0U;

    for (int line : lines) {
        if (line < 0) {
            os << NoteMsg{std::to_string(-line) + " lines folded"} << '\n';
            lineNo += -line;
        } else {
            std::string fileLine;
            if (!std::getline(ss, fileLine)) {
                // Not enough lines in the file.
                fileLine = "<<< EOF >>>";
                ++extraLines;
            }

            os << LineNo{{lineNo + 1U, lineNoWidth}}
               << covCol.active(lineNo) << ": " << fileLine << '\n';
            ++lineNo;
        }
    }

    // Print extra file lines (with unknown coverage).
    for (std::string fileLine; std::getline(ss, fileLine); ++lineNo) {
        os << LineNo{{lineNo + 1U, lineNoWidth}}
           << covCol.active(lineNo) << ": " << fileLine << '\n';
    }

    if (extraLines > 1U) {
        os << ErrorMsg{"ERROR"} << ": too few lines in the file.\n";
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
    std::stringstream fss, sss;
    highlight(fss, oText, lang, &fLines);
    highlight(sss, nText, lang, &sLines);

    auto getLine = [](std::stringstream &ss) {
        std::string line;
        std::getline(ss, line);
        return line;
    };

    ColorCane cc;
    CoverageColumn oldCovCol(oCov), newCovCol(nCov);
    for (const DiffLine &line : diff) {
        switch (line.type) {
            case DiffLineType::Added:
                cc << oldCovCol.blank() << ':'
                   << newCovCol.active(line.newLine) << ':'
                   << LineAdded{getLine(sss)};
                break;
            case DiffLineType::Removed:
                cc << oldCovCol.active(line.oldLine) << ':'
                   << newCovCol.blank() << ':'
                   << LineRemoved{getLine(fss)};
                break;
            case DiffLineType::Note:
                cc << NoteMsg{line.text};
                break;
            case DiffLineType::Common:
                cc << oldCovCol.active(line.oldLine) << ':'
                   << newCovCol.active(line.newLine) << ':'
                   << LineRetained{getLine(fss)};
                break;
            case DiffLineType::Identical:
                cc << oldCovCol.inactive(line.oldLine) << ':'
                   << newCovCol.inactive(line.newLine) << ':'
                   << LineRetained{getLine(fss)};
                break;
        }
        cc << '\n';
    }

    os << cc;
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

void
FilePrinter::highlight(std::stringstream &ss, std::istream &text,
                       const std::string &lang, srchilite::LineRanges *ranges)
{
    if (colorizeOutput) {
        highlighter.setLineRanges(ranges);
        highlighter.highlight(text, ss, lang);
    } else {
        int lineNo = 1;
        for (std::string line; std::getline(text, line); ++lineNo) {
            if (ranges == nullptr || ranges->isInRange(lineNo)) {
                ss << line << '\n';
            }
        }
    }
}
