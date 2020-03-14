// Copyright (C) 2020 xaizek <xaizek@posteo.net>
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

#include "Settings.hpp"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include <algorithm>
#include <string>
#include <utility>

namespace pt = boost::property_tree;

void
Settings::loadFromFile(const std::string &path)
{
    pt::ptree props;

    try {
        pt::read_ini(path, props);
    } catch (pt::ini_parser_error &) {
        // Silently ignore invalid or nonexistent configuration.
        return;
    }

    medLimit = props.get<float>("low-bound", medLimit);
    hiLimit = props.get<float>("hi-bound", hiLimit);
    tabSize = props.get<int>("tab-size", tabSize);
    setMinFoldSize(props.get<int>("min-fold-size", minFoldSize));
    foldContext = props.get<int>("fold-context", foldContext);
    setPrintLineNoInDiff(props.get<bool>("diff-show-lineno", diffShowLineNo));

    medLimit = std::max(0.0f, std::min(100.0f, medLimit));
    hiLimit = std::max(0.0f, std::min(100.0f, hiLimit));
    if (hiLimit < medLimit) {
        std::swap(medLimit, hiLimit);
    }

    tabSize = std::max(1, std::min(25, tabSize));
    foldContext = std::max(0, std::min(100, foldContext));
}

void
Settings::setMinFoldSize(int value)
{
    minFoldSize = std::max(1, std::min(100, value));
}

void
Settings::setPrintLineNoInDiff(bool value)
{
    diffShowLineNo = value;
}
