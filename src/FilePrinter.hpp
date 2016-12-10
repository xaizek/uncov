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

#include <string>
#include <vector>


class FilePrinter
{
public:
    FilePrinter();

public:
    void print(const std::string &path, const std::string &contents,
               const std::vector<int> &coverage);

private:
    srchilite::SourceHighlight sourceHighlight;
    srchilite::LangMap langMap;
};

#endif // UNCOVER__FILEPRINTER_HPP__
