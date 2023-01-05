// Copyright (C) 2021 xaizek <xaizek@posteo.net>
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

#ifndef UNCOV_APP_HPP_
#define UNCOV_APP_HPP_

#include <string>

/**
 * @file app.hpp
 *
 * @brief This unit provides values of user-visible constants.
 */

class Repository;

/**
 * @brief Retrieves version string in the form "v0.
 *
 * @returns The version.
 */
std::string getAppVersion();

/**
 * @brief Retrieves name of configuration file.
 *
 * @returns The name.
 */
std::string getConfigFile();

/**
 * @brief Retrieves name of database file.
 *
 * @returns The name.
 */
std::string getDatabaseFile();

/**
 * @brief Selects base path for local data during this run of the application.
 *
 * @param repo Repository to work with.
 *
 * @returns The path.
 */
std::string pickDataPath(const Repository &repo);

#endif // UNCOV_APP_HPP_
