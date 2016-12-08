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

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "decoration.hpp"

FilePrinter::FilePrinter() : sourceHighlight("esc256.outlang"),
                             langMap("lang.map")
{
    sourceHighlight.setStyleFile("esc256.style");
}

void
FilePrinter::print(const std::string &path, const std::string &contents,
                   const std::vector<int> &coverage)
{
    const int LineNoWidth = 5, HitsNumWidth = 5;

    std::string lang = langMap.getMappedFileNameFromFileName(path);
    if (lang.empty()) {
        lang = "cpp.lang";
    }

    std::istringstream iss(contents);
    std::stringstream ss;
    sourceHighlight.highlight(iss, ss, lang);

    auto countWidth = [](int n) {
        int width = 1;
        while (n > 0) {
            n /= 10;
            ++width;
        }
        return width;
    };

    const int nLines = coverage.size();
    const int lineNoWidth = std::max(LineNoWidth, countWidth(nLines));
    // XXX: this could in principle be stored in database.
    const int maxHits = *std::max_element(coverage.cbegin(),
                                          coverage.cend());
    const int hitsNumWidth = std::max(HitsNumWidth, countWidth(maxHits));

    std::size_t lineNo = 0;
    for (std::string fileLine; std::getline(ss, fileLine); ++lineNo) {
        std::cout << (decor::white_bg + decor::black_fg
                  << std::setw(lineNoWidth) << lineNo + 1 << ' ');

        if (lineNo < coverage.size()) {
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

            std::cout << (dec << std::setw(hitsNumWidth) << prefix << ' ');
        } else {
            std::cout << (decor::red_bg + decor::inv + decor::bold
                      << std::setw(hitsNumWidth) << "ERROR" << ' ');
        }
        std::cout << ": ";

        std::cout << fileLine << std::endl;
    }
}
