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

#ifndef UNCOVER__INVOCATION_HPP__
#define UNCOVER__INVOCATION_HPP__

#include <string>
#include <vector>

/**
 * @brief Breaks command-line arguments into separate fields.
 */
class Invocation
{
public:
    /**
     * @brief Parses argument list.
     *
     * @param args Arguments of the application.
     *
     * @throws std::invalid_argument on empty argument list.
     */
    explicit Invocation(std::vector<std::string> args);

public:
    /**
     * @brief Retrieves error message, if any.
     *
     * @returns Empty string if there is no error, otherwise error message.
     */
    const std::string & getError() const;

    /**
     * @brief Retrieves path to repository.
     *
     * @returns The path.
     */
    const std::string & getRepositoryPath() const;

    /**
     * @brief Retrieves name of subcommand to execute.
     *
     * @returns The name.
     */
    const std::string & getSubcommandName() const;

    /**
     * @brief Retrieves arguments for subcommand.
     *
     * @returns The arguments.
     */
    const std::vector<std::string> & getSubcommandArgs() const;

public:
    /**
     * @brief Error message or empty string.
     */
    std::string error;
    /**
     * @brief Path to repository.
     */
    std::string repositoryPath;
    /**
     * @brief Name of subcommand to execute.
     */
    std::string subcommandName;
    /**
     * @brief Arguments for subcommand.
     */
    std::vector<std::string> subcommandArgs;
};

#endif // UNCOVER__INVOCATION_HPP__
