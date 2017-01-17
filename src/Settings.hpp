// Copyright (C) 2017 xaizek <xaizek@openmailbox.org>
//
// This file is part of uncov.
//
// uncov is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// uncov is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with uncov.  If not, see <http://www.gnu.org/licenses/>.

#ifndef UNCOV__SETTINGS_HPP__
#define UNCOV__SETTINGS_HPP__

#include <string>

#include "printing.hpp"

class Settings : public PrintingSettings
{
    // XXX: hard-coded values.

public:
    virtual std::string getTimeFormat() const override
    {
        return "%Y-%m-%d %H:%M:%S";
    }

    virtual float getMedLimit() const override
    {
        return 70.0f;
    }

    virtual float getHiLimit() const override
    {
        return 90.0f;
    }
};

#endif // UNCOV__SETTINGS_HPP__
