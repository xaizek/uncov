// Copyright (C) 2020 xaizek <xaizek@posteo.net>
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

#ifndef UNCOV__UNCOV_HPP__
#define UNCOV__UNCOV_HPP__

#include <string>
#include <vector>

#include "Invocation.hpp"

class Settings;

/**
 * @brief Class that represents application.
 */
class Uncov
{
public:
    /**
     * @brief Constructs main application class and parses arguments.
     *
     * @param args Arguments of the application.
     *
     * @throws std::invalid_argument on empty argument list.
     */
    explicit Uncov(std::vector<std::string> args);

public:
    /**
     * @brief Entry point of the application.
     *
     * @param settings Configuration.
     *
     * @returns Exit status of the application (to be returned by @c main()).
     */
    int run(Settings &settings);

private:
    Invocation invocation; //!< Processor of command-line arguments.
};

#endif // UNCOV__UNCOV_HPP__
