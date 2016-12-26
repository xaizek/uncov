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

#ifndef UNCOVER__FILECOMPARATOR_HPP__
#define UNCOVER__FILECOMPARATOR_HPP__

#include <deque>
#include <string>
#include <vector>

enum class DiffLineType
{
    Note,
    Common,
    Identical,
    Added,
    Removed
};

struct DiffLine
{
    DiffLineType type;
    std::string text;
    int oldLine;
    int newLine;

    DiffLine(DiffLineType type, const std::string &text,
             int oldLine, int newLine)
        : type(type), text(text), oldLine(oldLine), newLine(newLine)
    {
    }

    int getLine() const
    {
        return oldLine > newLine ? oldLine : newLine;
    }
};

class FileComparator
{
public:
    FileComparator(const std::vector<std::string> &o,
                   const std::vector<int> &oCov,
                   const std::vector<std::string> &n,
                   const std::vector<int> &nCov,
                   bool considerHits);

public:
    bool isValidInput() const;
    std::string getInputError() const;
    bool areEqual() const;
    const std::deque<DiffLine> & getDiffSequence() const;

private:
    bool valid;
    std::string inputError;
    bool equal;
    std::deque<DiffLine> diffSequence;
};

#endif // UNCOVER__FILECOMPARATOR_HPP__
