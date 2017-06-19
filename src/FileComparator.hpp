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

#ifndef UNCOV__FILECOMPARATOR_HPP__
#define UNCOV__FILECOMPARATOR_HPP__

#include <deque>
#include <string>
#include <utility>
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

    DiffLine(DiffLineType type, std::string text,
             int oldLine = -1, int newLine = -1)
        : type(type), text(text), oldLine(oldLine), newLine(newLine)
    {
    }

    int getLine() const
    {
        return oldLine > newLine ? oldLine : newLine;
    }
};

/**
 * @brief FileComparator-specific settings.
 */
class FileComparatorSettings
{
public:
    /**
     * @brief Enable polymorphic destruction.
     */
    virtual ~FileComparatorSettings() = default;

public:
    /**
     * @brief Retrieves minimum size of a fold.
     *
     * @returns The size.
     */
    virtual int getMinFoldSize() const = 0;

    /**
     * @brief Retrieves size of context in diff results.
     *
     * Both above and below folded piece.
     *
     * @returns The size.
     */
    virtual int getDiffContext() const = 0;
};

class FileComparator
{
public:
    FileComparator(const std::vector<std::string> &o,
                   const std::vector<int> &oCov,
                   const std::vector<std::string> &n,
                   const std::vector<int> &nCov,
                   bool considerHits,
                   const FileComparatorSettings &settings);

public:
    bool isValidInput() const;
    std::string getInputError() const;
    bool areEqual() const;
    const std::deque<DiffLine> & getDiffSequence() const;

private:
    bool valid;
    std::string inputError;
    bool equal;
    std::deque<DiffLine> diffSeq;
};

#endif // UNCOV__FILECOMPARATOR_HPP__
