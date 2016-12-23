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

#include "printing.hpp"

#include <cstddef>

#include <iomanip>
#include <ostream>
#include <string>

#include "decoration.hpp"

std::ostream &
operator<<(std::ostream &os, const CLinesChange &change)
{
    if (change.data < 0) {
        os << decor::bold + decor::red_fg;
    } else if (change.data == 0) {
        os << decor::bold + decor::black_fg;
    } else {
        os << decor::bold + decor::green_fg << std::showpos;
    }
    return os << change.data << decor::def << std::noshowpos;
}

std::ostream &
operator<<(std::ostream &os, const ULinesChange &change)
{
    const auto width = os.width();
    if (change.data > 0) {
        os << decor::bold + decor::red_fg << std::showpos;
    } else if (change.data == 0) {
        os << decor::bold + decor::black_fg;
    } else {
        os << decor::bold + decor::green_fg;
    }
    return os << std::setw(width) << change.data << decor::def
              << std::noshowpos;
}

std::ostream &
operator<<(std::ostream &os, const RLinesChange &change)
{
    const auto width = os.width();
    if (change.data > 0) {
        os << std::showpos;
    }
    return os << (decor::yellow_fg << std::setw(width) << change.data)
              << std::noshowpos;
}

std::ostream &
operator<<(std::ostream &os, const CoverageChange &change)
{
    if (change.data < 0) {
        os << decor::bold + decor::red_fg;
    } else if (change.data == 0) {
        os << decor::bold + decor::black_fg;
    } else {
        os << decor::bold + decor::green_fg << '+';
    }
    return os << change.data << '%' << decor::def;
}

std::ostream &
operator<<(std::ostream &os, const Coverage &coverage)
{
    // XXX: hard-coded coverage thresholds.
    if (coverage.data < 70.0f) {
        os << decor::bold + decor::red_fg;
    } else if (coverage.data < 90.0f) {
        os << decor::bold + decor::yellow_fg;
    } else {
        os << decor::bold + decor::green_fg;
    }
    return os << coverage.data << '%' << decor::def;
}

std::ostream &
operator<<(std::ostream &os, const Label &label)
{
    return os << (decor::bold << label.data);
}

std::ostream &
operator<<(std::ostream &os, const ErrorMsg &errMsg)
{
    return os << (decor::red_bg + decor::inv + decor::bold << errMsg.data);
}

std::ostream &
operator<<(std::ostream &os, const TableHeader &th)
{
    return os << (decor::white_fg + decor::black_bg + decor::bold + decor::inv
              << th.data);
}

std::ostream &
operator<<(std::ostream &os, const LineNo &lineNo)
{
    return os << (decor::white_bg + decor::black_fg << lineNo.data << ' ');
}

std::ostream &
operator<<(std::ostream &os, const LineAdded &line)
{
    return os << (decor::green_fg + decor::bold << '+') << line.data;
}

std::ostream &
operator<<(std::ostream &os, const LineRemoved &line)
{
    return os << (decor::red_fg + decor::bold << '-') << line.data;
}

std::ostream &
operator<<(std::ostream &os, const HitsCount &hits)
{
    if (hits.data == 0) {
        os << (decor::red_fg + decor::inv + decor::bold << "x0" << ' ');
    } else if (hits.data > 0) {
        os << (decor::green_fg + decor::inv + decor::bold
           << 'x' + std::to_string(hits.data) << ' ');
    } else {
        os << "" << ' ';
    }
    return os;
}
