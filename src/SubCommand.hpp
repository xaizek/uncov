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

#ifndef UNCOV_SUBCOMMAND_HPP_
#define UNCOV_SUBCOMMAND_HPP_

#include <cstddef>

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

/**
 * @file SubCommand.hpp
 *
 * @brief This unit provides base for implementing sub-commands.
 */

class BuildHistory;
class Repository;
class Settings;
class Uncov;

/**
 * @brief Base class for all sub-commands.
 */
class SubCommand
{
public:
    /**
     * @brief Retrieves list of all registered sub-commands.
     *
     * @returns The list.
     */
    static std::vector<SubCommand *> getAll();

protected:
    /**
     * @brief Registers default constructed instance of specified subclass.
     *
     * @tparam C Subclass type.
     */
    template <typename C>
    static void registerCmd()
    {
        getAllCmds().emplace_back(new C());
    }

private:
    /**
     * @brief Retrieves reference to static list of commands.
     *
     * @returns The reference.
     */
    static std::vector<std::unique_ptr<SubCommand>> & getAllCmds();

public:
    /**
     * @brief Constructs command which takes variadic number of arguments.
     *
     * @param names   List of command aliases.
     * @param minArgs Minimum number of arguments allowed.
     * @param maxArgs Maximum number of arguments allowed.
     */
    SubCommand(std::vector<std::string> names,
               std::size_t minArgs, std::size_t maxArgs)
        : names(std::move(names)), minArgs(minArgs), maxArgs(maxArgs)
    {
    }

    /**
     * @brief Constructs command which takes zero or N arguments.
     *
     * @param names List of command aliases.
     * @param nArgs Required number of arguments.
     */
    SubCommand(std::vector<std::string> names, std::size_t nArgs = 0U)
        : SubCommand(std::move(names), nArgs, nArgs)
    {
    }

    /**
     * @brief Checks that all aliases have descriptions.
     */
    virtual ~SubCommand();

public:
    /**
     * @brief Retrieves list of aliases of this command.
     *
     * @returns The list.
     */
    const std::vector<std::string> & getNames() const
    {
        return names;
    }

    /**
     * @brief Retrieves description of an alias.
     *
     * @param alias An alias.
     *
     * @returns The description.
     */
    const std::string & getDescription(const std::string &alias) const
    {
        return descriptions.at(alias);
    }

    /**
     * @brief Checks whether this command is of application-level.
     *
     * Application-commands don't need user's data (repository, etc.).
     *
     * @returns @c true if so, @c false otherwise.
     */
    virtual bool isGeneric() const
    {
        return false;
    }

    /**
     * @brief Prints help for this command.
     *
     * @param os    Output stream.
     * @param alias Alias of the command.
     */
    virtual void printHelp(std::ostream &os,
                           const std::string &alias) const = 0;

    /**
     * @brief Runs app sub-command (doesn't require user's data).
     *
     * @param uncov Reference to the application.
     * @param alias Alias of the command.
     * @param args  Arguments.
     *
     * @returns Either @c EXIT_FAILURE or @c EXIT_SUCCESS.
     *
     * @throws std::logic_error If this is a repo sub-command.
     */
    int exec(Uncov &uncov, const std::string &alias,
             const std::vector<std::string> &args);

    /**
     * @brief Runs repo sub-command (requires user's data).
     *
     * @param settings Configuration.
     * @param bh       Build history.
     * @param repo     Repository to work on.
     * @param alias    Alias of the command.
     * @param args     Arguments.
     *
     * @returns Either @c EXIT_FAILURE or @c EXIT_SUCCESS.
     *
     * @throws std::logic_error If this is an app sub-command.
     */
    int exec(Settings &settings, BuildHistory &bh, Repository &repo,
             const std::string &alias, const std::vector<std::string> &args);

protected:
    /**
     * @brief Sets description for an alias.
     *
     * @param alias An alias.
     * @param descr Description of the alias.
     *
     * @throws std::logic_error On wrong or already described alias.
     */
    void describe(const std::string &alias, const std::string &descr);

    /**
     * @brief Signals that an error has occurred.
     */
    void error();

    /**
     * @brief Checks whether an error has occurred.
     *
     * @returns @c true if so, @c false otherwise.
     */
    bool isFailed() const
    {
        return hasErrors;
    }

protected:
    //! @name This is more convenient equivalent of get*() functions.
    //! @brief It allows execImpl() to have less arguments.
    //! @{
    Settings *const &settings = settingsValue; //!< Configuration.
    BuildHistory *const &bh = bhValue;         //!< Build history.
    Repository *const &repo = repoValue;       //!< Repository.
    Uncov *const &uncov = uncovValue;          //!< Application.
    //! @}

private:
    /**
     * @brief Checks correctness of the invocation.
     *
     * Calls `error()` and prints error messages to standard output.
     *
     * @param alias Alias of the command.
     * @param args  Arguments.
     */
    void checkExec(const std::string &alias,
                   const std::vector<std::string> &args);
    /**
     * @brief Formats part of error message about number of expected arguments.
     *
     * @returns The message.
     */
    std::string makeExpectedMsg() const;
    /**
     * @brief Checks whether specified alias is associated with this command.
     *
     * @param alias Alias.
     *
     * @returns @c true if so, @c false otherwise.
     */
    bool isAlias(const std::string &alias) const;

private:
    /**
     * @brief Implementation of the command.
     *
     * @param alias Name with which this command was called.
     * @param args  Arguments passed to the command.
     */
    virtual void execImpl(const std::string &alias,
                          const std::vector<std::string> &args) = 0;

private:
    const std::vector<std::string> names; //!< Aliases of this command.
    const std::size_t minArgs;            //!< Minimum number of arguments.
    const std::size_t maxArgs;            //!< Maximum number of arguments.

    //! Descriptions of the aliases.
    std::unordered_map<std::string, std::string> descriptions;

    int hasErrors = false;             //!< Whether an error has occurred.
    Settings *settingsValue = nullptr; //!< Configuration.
    BuildHistory *bhValue = nullptr;   //!< Build history.
    Repository *repoValue = nullptr;   //!< Repository.
    Uncov *uncovValue = nullptr;       //!< Application.
};

#endif // UNCOV_SUBCOMMAND_HPP_
