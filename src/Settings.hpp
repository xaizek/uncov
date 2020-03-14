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

#ifndef UNCOV__SETTINGS_HPP__
#define UNCOV__SETTINGS_HPP__

#include <string>

#include "FileComparator.hpp"
#include "FilePrinter.hpp"
#include "integration.hpp"
#include "printing.hpp"

/**
 * @file Settings.hpp
 *
 * @brief Implementation of settings.
 */

/**
 * @brief Implementation of settings for all classes that have them.
 */
class Settings : public PrintingSettings, public FilePrinterSettings,
                 public FileComparatorSettings
{
    // XXX: hard-coded values.

public: // PrintingSettings only
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

public: // FilePrinterSettings only
    virtual int getTabSize() const override
    {
        return 4;
    }

    virtual bool isColorOutputAllowed() const override
    {
        return isOutputToTerminal();
    }

    virtual bool printLineNoInDiff() const override
    {
        return false;
    }

public: // FileComparatorSettings only
    virtual int getMinFoldSize() const override
    {
        return 3;
    }

    virtual int getFoldContext() const override
    {
        return 1;
    }

public: // PrintingSettings and FilePrinterSettings
    virtual bool isHtmlOutput() const override
    {
        return false;
    }
};

#endif // UNCOV__SETTINGS_HPP__
