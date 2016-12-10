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

#ifndef UNCOVER__FILEPRINTER_HPP__
#define UNCOVER__FILEPRINTER_HPP__

#include <srchilite/sourcehighlight.h>
#include <srchilite/langmap.h>

#include <cstddef>

#include <sstream>
#include <string>
#include <vector>

class Text
{
public:
    Text(const std::string &text);

public:
    const std::vector<std::string> & asLines();
    std::istringstream & asStream();
    std::size_t size();

private:
    std::vector<std::string> lines;
    std::istringstream iss;
};

class FilePrinter
{
public:
    FilePrinter();

public:
    void print(const std::string &path, const std::string &contents,
               const std::vector<int> &coverage);

    /**
     * @brief Finds and prints differences between two versions of a file.
     *
     * Implements solution for longest common subsequence problem that matches
     * modified finding of edit distance (substitution operation excluded) with
     * backtracking afterward to compose result.  Requires lots of memory for
     * very big files.
     *
     * @param path Name of the file (for highlighting detection).
     * @param oText Old version of the file.
     * @param oCov Coverage of old version.
     * @param nText New version of the file.
     * @param nCov Coverage of new version.
     *
     * @note `oText.size() == oCov.size() && nText.size() == nCov.size()` is
     *       assumed.
     */
    void printDiff(const std::string &path,
                   Text &oText, const std::vector<int> &oCov,
                   Text &nText, const std::vector<int> &nCov);

private:
    std::string getLang(const std::string &path);

private:
    srchilite::SourceHighlight sourceHighlight;
    srchilite::LangMap langMap;
};

#endif // UNCOVER__FILEPRINTER_HPP__
