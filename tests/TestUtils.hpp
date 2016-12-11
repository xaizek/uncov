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

#ifndef UNCOVER__TESTS__TESTUTILS_HPP__
#define UNCOVER__TESTS__TESTUTILS_HPP__

#include <string>

class TempDirCopy
{
public:
    TempDirCopy(const std::string &from, const std::string &to);

    TempDirCopy(const TempDirCopy &) = delete;
    TempDirCopy & operator=(const TempDirCopy &) = delete;

    ~TempDirCopy();

private:
    const std::string to;
};

#endif // UNCOVER__TESTS__TESTUTILS_HPP__
