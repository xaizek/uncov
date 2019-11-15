// Copyright (C) 2018 xaizek <xaizek@posteo.net>
//
// This file is part of uncov.
//
// uncov is free software: you can redistribute it and/or modify
// it under the terms of version 3 of the GNU Affero General Public License as
// published by the Free Software Foundation.
//
// uncov is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with uncov.  If not, see <http://www.gnu.org/licenses/>.

#include "ColorCane.hpp"

#include <boost/utility/string_ref.hpp>

#include <string>
#include <vector>

void
ColorCane::append(boost::string_ref text, ColorGroup hi)
{
    pieces.emplace_back(text.to_string(), hi);
}

void
ColorCane::append(char text, ColorGroup hi)
{
    pieces.emplace_back(std::string(1, text), hi);
}

ColorCane::Pieces::const_iterator
ColorCane::begin() const
{
    return pieces.begin();
}

ColorCane::Pieces::const_iterator
ColorCane::end() const
{
    return pieces.end();
}
