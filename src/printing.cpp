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
#include <unordered_map>
#include <utility>

#include "decoration.hpp"

namespace {

const std::unordered_map<std::string, decor::Decoration> highlightGroups = {
    { "linesbad",      decor::bold + decor::red_fg   },
    { "linesok",       decor::bold + decor::black_fg },
    { "linesgood",     decor::bold + decor::green_fg },
    { "lineschanged",  decor::yellow_fg },
    { "covbad",        decor::bold + decor::red_fg },
    { "covok",         decor::bold + decor::black_fg },
    { "covnormal",     decor::bold + decor::yellow_fg },
    { "covgood",       decor::bold + decor::green_fg },
    { "lineno",        decor::white_bg + decor::black_fg },
    { "missed",        decor::red_fg + decor::inv + decor::bold },
    { "covered",       decor::green_fg + decor::inv + decor::bold },
    { "silentmissed",  decor::red_fg + decor::bold },
    { "silentcovered", decor::green_fg + decor::bold },
    { "added",         decor::green_fg + decor::bold },
    { "removed",       decor::red_fg + decor::bold },
    { "header",        decor::white_fg + decor::black_bg + decor::bold +
                       decor::inv },
    { "error",         decor::red_bg + decor::inv + decor::bold },
    { "label",         decor::bold },
    { "revision",      decor::none },
    { "time",          decor::none },
};

class Highlight
{
public:
    Highlight(std::string groupName) : groupName(std::move(groupName))
    {
    }

    Highlight(Highlight &&hi, std::function<void(std::ostream&)> app)
        : groupName(std::move(hi.groupName)), apps(std::move(hi.apps))
    {
        apps.push_back(app);
    }

public:
    std::ostream & decorate(std::ostream &os) const
    {
        os << highlightGroups.at(groupName);

        for (const auto app : apps) {
            app(os);
        }

        os << decor::def;

        return os;
    }

private:
    const std::string groupName;
    std::vector<std::function<void(std::ostream&)>> apps;
};

template <typename T>
Highlight
operator<<(Highlight &&hi, const T &val)
{
    return Highlight(std::move(hi), [val](std::ostream &os) { os << val; });
}

inline std::ostream &
operator<<(std::ostream &os, const Highlight &hi)
{
    return hi.decorate(os);
}

std::ostream &
printHits(std::ostream &os, int hits, bool silent)
{
    std::string prefix = silent ? "silent" : "";

    if (hits == 0) {
        return os << (Highlight(prefix + "missed") << "x0" << ' ');
    } else if (hits > 0) {
        // 'x' and number must be output as a single unit here so that field
        // width applies correctly.
        return os << (Highlight(prefix + "covered")
                  << 'x' + std::to_string(hits) << ' ');
    } else {
        return os << "" << ' ';
    }
}

}

std::ostream &
operator<<(std::ostream &os, const CLinesChange &change)
{
    if (change.data < 0) {
        return os << (Highlight("linesbad") << change.data);
    } else if (change.data == 0) {
        return os << (Highlight("linesok") << change.data);
    } else {
        return os << std::showpos
                  << (Highlight("linesgood") << change.data)
                  << std::noshowpos;
    }
}

std::ostream &
operator<<(std::ostream &os, const ULinesChange &change)
{
    if (change.data > 0) {
        return os << std::showpos
                  << (Highlight("linesbad") << change.data)
                  << std::noshowpos;
    } else if (change.data == 0) {
        return os << (Highlight("linesok") << change.data);
    } else {
        return os << (Highlight("linesgood") << change.data);
    }
}

std::ostream &
operator<<(std::ostream &os, const RLinesChange &change)
{
    if (change.data > 0) {
        os << std::showpos;
    }
    return os << (Highlight("lineschanged") << change.data) << std::noshowpos;
}

std::ostream &
operator<<(std::ostream &os, const CoverageChange &change)
{
    if (change.data < 0) {
        return os << (Highlight("covbad") << change.data << '%');
    } else if (change.data == 0) {
        return os << (Highlight("covok") << change.data << '%');
    } else {
        return os << std::showpos
                  << (Highlight("covgood") << change.data << '%')
                  << std::noshowpos;
    }
}

std::ostream &
operator<<(std::ostream &os, const Coverage &coverage)
{
    // XXX: hard-coded coverage thresholds.

    if (coverage.data < 70.0f) {
        return os << (Highlight("covbad") << coverage.data << '%');
    } else if (coverage.data < 90.0f) {
        return os << (Highlight("covnormal") << coverage.data << '%');
    } else {
        return os << (Highlight("covgood") << coverage.data << '%');
    }
}

std::ostream &
operator<<(std::ostream &os, const Label &label)
{
    return os << (Highlight("label") << label.data);
}

std::ostream &
operator<<(std::ostream &os, const ErrorMsg &errMsg)
{
    return os << (Highlight("error") << errMsg.data);
}

std::ostream &
operator<<(std::ostream &os, const TableHeader &th)
{
    return os << (Highlight("header") << th.data);
}

std::ostream &
operator<<(std::ostream &os, const LineNo &lineNo)
{
    return os << (Highlight("lineno") << lineNo.data << ' ');
}

std::ostream &
operator<<(std::ostream &os, const LineAdded &line)
{
    return os << (Highlight("added") << '+') << line.data;
}

std::ostream &
operator<<(std::ostream &os, const LineRemoved &line)
{
    return os << (Highlight("removed") << '-') << line.data;
}

std::ostream &
operator<<(std::ostream &os, const HitsCount &hits)
{
    return printHits(os, hits.data, false);
}

std::ostream &
operator<<(std::ostream &os, const SilentHitsCount &hits)
{
    return printHits(os, hits.data, true);
}

std::ostream &
operator<<(std::ostream &os, const Revision &rev)
{
    return os << (Highlight("revision") << rev.data);
}

std::ostream &
operator<<(std::ostream &os, const Time &t)
{
    // XXX: hard-coded time format.
    return os << (Highlight("time")
              << std::put_time(std::localtime(&t.data), "%Y-%m-%d %H:%M:%S"));
}
