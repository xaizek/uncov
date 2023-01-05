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

#ifndef UNCOV_SETTINGS_HPP_
#define UNCOV_SETTINGS_HPP_

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
public:
    // Loads some of the settings from file.  Does nothing if it doesn't exist.
    void loadFromFile(const std::string &path);

public: // PrintingSettings only
    virtual std::string getTimeFormat() const override
    {
        return "%Y-%m-%d %H:%M:%S";
    }

    virtual float getMedLimit() const override
    {
        return medLimit;
    }

    virtual float getHiLimit() const override
    {
        return hiLimit;
    }

public: // FilePrinterSettings only
    virtual int getTabSize() const override
    {
        return tabSize;
    }

    virtual bool isColorOutputAllowed() const override
    {
        return isOutputToTerminal();
    }

    virtual bool printLineNoInDiff() const override
    {
        return diffShowLineNo;
    }

public: // FileComparatorSettings and FilePrinterSettings
    virtual int getMinFoldSize() const override
    {
        return minFoldSize;
    }

    virtual int getFoldContext() const override
    {
        return foldContext;
    }

public: // PrintingSettings and FilePrinterSettings
    virtual bool isHtmlOutput() const override
    {
        return false;
    }

protected:
    /**
     * @brief Sets minimal size of a fold.
     *
     * @param value New value of the threshold.
     */
    void setMinFoldSize(int value);

    /**
     * @brief Configures printing line numbers in diff.
     *
     * @param value New value of the flag.
     */
    void setPrintLineNoInDiff(bool value);

private:
    //! Percentage boundary between low and medium coverage.
    float medLimit = 70.0f;
    //! Percentage boundary between medium and high coverage.
    float hiLimit = 90.0f;
    //! Number of spaces in a full tabulation.
    int tabSize = 4;
    //! Minimal number of lines to be folded.
    int minFoldSize = 3;
    //! Whether line numbers are displayed in diffs.
    bool diffShowLineNo = false;
    //! Number of visible lines above and below interesting lines.
    int foldContext = 1;
};

#endif // UNCOV_SETTINGS_HPP_
