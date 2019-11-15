// Copyright (C) 2016 xaizek <xaizek@posteo.net>
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

#ifndef UNCOV__INTEGRATION_HPP__
#define UNCOV__INTEGRATION_HPP__

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "utils/Flag.hpp"

/**
 * @file integration.hpp
 *
 * @brief Several terminal and environment integration utilities.
 */

/**
 * @brief A class that automatically spawns pager if output is large.
 *
 * Output must come to @c std::cout and is considered to be large when it
 * doesn't fit screen height.
 */
class RedirectToPager
{
public:
    class Impl;

    /**
     * @brief Can redirect @c std::cout until destruction.
     */
    RedirectToPager();

    //! No copy-constructor.
    RedirectToPager(const RedirectToPager &rhs) = delete;
    //! No copy-move.
    RedirectToPager & operator=(const RedirectToPager &rhs) = delete;

    /**
     * @brief Restores previous state of @c std::cout.
     */
    ~RedirectToPager();

private:
    //! Implementation details.
    std::unique_ptr<Impl> impl;
};

//! Boolean flag type for controlling capturing of @c stderr.
using CatchStderr = Flag<struct CatchStderrTag>;

/**
 * @brief Runs external command for its exit code.
 *
 * @param cmd Command to run.
 * @param dir Directory to run the command in.
 * @param catchStdErr Whether to redirect @c stderr.
 *
 * @returns Exit code of the command.
 *
 * @throws std::runtime_error On failure to run the command or when it didn't
 *                            finish properly.
 */
int queryProc(std::vector<std::string> &&cmd, const std::string &dir = ".",
              CatchStderr catchStdErr = CatchStderr(false));

/**
 * @brief Runs external command for its output.
 *
 * @param cmd Command to run.
 * @param dir Directory to run the command in.
 * @param catchStdErr Whether to redirect @c stderr.
 *
 * @returns Exit code of the command.
 *
 * @throws std::runtime_error On failure to run the command or when it didn't
 *                            finish properly.
 */
std::string readProc(std::vector<std::string> &&cmd,
                     const std::string &dir = ".",
                     CatchStderr catchStdErr = CatchStderr(false));

/**
 * @brief Queries whether program output is connected to terminal.
 *
 * @returns @c true if so, otherwise @c false.
 */
bool isOutputToTerminal();

/**
 * @brief Retrieves terminal width and height in characters.
 *
 * @returns Pair of actual terminal width and height, or maximum possible values
 *          of the type.
 */
std::pair<unsigned int, unsigned int> getTerminalSize();

#endif // UNCOV__INTEGRATION_HPP__
