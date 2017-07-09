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

#include "decoration.hpp"

#include <ostream>

#include "integration.hpp"

namespace {

/**
 * @brief Class that manages global state of this unit.
 */
class ColorsState
{
public:
    /**
     * @brief Disables highlighting.
     */
    void disable() { isAscii = false; }

    /**
     * @brief Transforms ASCII-sequence to disable it when colorization is off.
     *
     * @param text String with ASCII-sequence.
     *
     * @returns Either @p text or an empty string.
     */
    const char * operator()(const char text[]) const
    {
        return isAscii ? text : "";
    }

private:
    //! Whether highlighting is enabled.
    bool isAscii = isOutputToTerminal();
};

}

// Shorten type name to fit into 80 columns limit.
using ostr = std::ostream;

using namespace decor;

//! Global state of this unit.
static ColorsState S;

const Decoration
    decor::none,
    decor::bold       ([](ostr &os) -> ostr & { return os << S("\033[1m"); }),
    decor::inv        ([](ostr &os) -> ostr & { return os << S("\033[7m"); }),
    decor::def        ([](ostr &os) -> ostr & { return os << S("\033[0m"); }),

    decor::black_fg   ([](ostr &os) -> ostr & { return os << S("\033[30m"); }),
    decor::red_fg     ([](ostr &os) -> ostr & { return os << S("\033[31m"); }),
    decor::green_fg   ([](ostr &os) -> ostr & { return os << S("\033[32m"); }),
    decor::yellow_fg  ([](ostr &os) -> ostr & { return os << S("\033[33m"); }),
    decor::blue_fg    ([](ostr &os) -> ostr & { return os << S("\033[34m"); }),
    decor::magenta_fg ([](ostr &os) -> ostr & { return os << S("\033[35m"); }),
    decor::cyan_fg    ([](ostr &os) -> ostr & { return os << S("\033[36m"); }),
    decor::white_fg   ([](ostr &os) -> ostr & { return os << S("\033[37m"); }),

    decor::black_bg   ([](ostr &os) -> ostr & { return os << S("\033[40m"); }),
    decor::red_bg     ([](ostr &os) -> ostr & { return os << S("\033[41m"); }),
    decor::green_bg   ([](ostr &os) -> ostr & { return os << S("\033[42m"); }),
    decor::yellow_bg  ([](ostr &os) -> ostr & { return os << S("\033[43m"); }),
    decor::blue_bg    ([](ostr &os) -> ostr & { return os << S("\033[44m"); }),
    decor::magenta_bg ([](ostr &os) -> ostr & { return os << S("\033[45m"); }),
    decor::cyan_bg    ([](ostr &os) -> ostr & { return os << S("\033[46m"); }),
    decor::white_bg   ([](ostr &os) -> ostr & { return os << S("\033[47m"); });

Decoration::Decoration(const Decoration &rhs)
    : decorator(rhs.decorator),
      lhs(rhs.lhs == nullptr ? nullptr : new Decoration(*rhs.lhs)),
      rhs(rhs.rhs == nullptr ? nullptr : new Decoration(*rhs.rhs))
{
}

Decoration::Decoration(decorFunc decorator) : decorator(decorator)
{
}

Decoration::Decoration(const Decoration &lhs, const Decoration &rhs)
    : lhs(new Decoration(lhs)),
      rhs(new Decoration(rhs))
{
}

std::ostream &
Decoration::decorate(std::ostream &os) const
{
    if (decorator != nullptr) {
        // Reset and preserve width field, so printing escape sequence doesn't
        // mess up formatting.
        const auto width = os.width({});
        os << decorator;
        static_cast<void>(os.width(width));
        return os;
    }
    if (lhs != nullptr && rhs != nullptr) {
        return os << *lhs << *rhs;
    }
    return os;
}

void
decor::disableDecorations()
{
    S.disable();
}
