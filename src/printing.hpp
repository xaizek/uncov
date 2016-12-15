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

#ifndef UNCOVER__PRINTING_HPP__
#define UNCOVER__PRINTING_HPP__

#include <ostream>
#include <string>

#include "decoration.hpp"

/**
 * @brief Strong typing of integer meaning covered lines change.
 */
struct CLinesChange { const int &data; };

/**
 * @brief Strong typing of integer meaning uncovered lines change.
 */
struct ULinesChange { const int &data; };

/**
 * @brief Strong typing of integer meaning relevant lines change.
 */
struct RLinesChange { const int &data; };

/**
 * @brief Strong typing of float meaning coverage change in percents.
 */
struct CoverageChange { const float &data; };

/**
 * @brief Strong typing of float meaning coverage in percents.
 */
struct Coverage { const float &data; };

/**
 * @brief Formatted covered lines change printer.
 *
 * @param os Stream to output formatted data to.
 * @param change Covered lines change.
 *
 * @returns @p os.
 */
inline std::ostream &
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

/**
 * @brief Formatted uncovered lines change printer.
 *
 * @param os Stream to output formatted data to.
 * @param change Uncovered lines change.
 *
 * @returns @p os.
 */
inline std::ostream &
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

/**
 * @brief Formatted relevant lines change printer.
 *
 * @param os Stream to output formatted data to.
 * @param change Relevant lines change.
 *
 * @returns @p os.
 */
inline std::ostream &
operator<<(std::ostream &os, const RLinesChange &change)
{
    const auto width = os.width();
    if (change.data > 0) {
        os << std::showpos;
    }
    return os << (decor::yellow_fg << std::setw(width) << change.data)
              << std::noshowpos;
}

/**
 * @brief Formatted coverage change printer.
 *
 * @param os Stream to output formatted data to.
 * @param change Coverage change in percents.
 *
 * @returns @p os.
 */
inline std::ostream &
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

/**
 * @brief Formatted coverage printer.
 *
 * @param os Stream to output formatted data to.
 * @param change Coverage in percents.
 *
 * @returns @p os.
 */
inline std::ostream &
operator<<(std::ostream &os, const Coverage &coverage)
{
    // XXX: hardcoded coverage thresholds.
    if (coverage.data < 70.0f) {
        os << decor::bold + decor::red_fg;
    } else if (coverage.data < 90.0f) {
        os << decor::bold + decor::yellow_fg;
    } else {
        os << decor::bold + decor::green_fg;
    }
    return os << coverage.data << '%' << decor::def;
}

#endif // UNCOVER__PRINTING_HPP__
