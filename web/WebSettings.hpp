// Copyright (C) 2017 xaizek <xaizek@posteo.net>
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

#ifndef UNCOV_WEB_WEBSETTINGS_HPP_
#define UNCOV_WEB_WEBSETTINGS_HPP_

#include "Settings.hpp"

class WebSettings : public Settings
{
public:
    WebSettings()
    {
        setMinFoldSize(4);
        setPrintLineNoInDiff(true);
    }

public: // FilePrinterSettings only
    virtual bool isColorOutputAllowed() const override
    {
        return true;
    }

public: // PrintingSettings and FilePrinterSettings
    virtual bool isHtmlOutput() const override
    {
        return true;
    }
};

#endif // UNCOV_WEB_WEBSETTINGS_HPP_
