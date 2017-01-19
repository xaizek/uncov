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

#ifndef UNCOV__SUBCOMMAND_HPP__
#define UNCOV__SUBCOMMAND_HPP__

#include <cstddef>

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class BuildHistory;
class Repository;
class Settings;

class SubCommand
{
public:
    static std::vector<SubCommand *> getAll();

protected:
    template <typename C>
    static void registerCmd()
    {
        getAllCmds().emplace_back(new C());
    }

private:
    static std::vector<std::unique_ptr<SubCommand>> & getAllCmds();

public:
    SubCommand(std::vector<std::string> names,
               std::size_t minArgs, std::size_t maxArgs)
        : names(std::move(names)), minArgs(minArgs), maxArgs(maxArgs)
    {
    }

    SubCommand(std::vector<std::string> names, std::size_t nArgs = 0U)
        : SubCommand(std::move(names), nArgs, nArgs)
    {
    }

    virtual ~SubCommand();

public:
    const std::vector<std::string> & getNames() const
    {
        return names;
    }

    const std::string & getDescription(const std::string &alias) const
    {
        return descriptions.at(alias);
    }

    int exec(Settings &settings, BuildHistory &bh, Repository &repo,
             const std::string &alias, const std::vector<std::string> &args);

protected:
    void describe(const std::string &alias, const std::string &descr);

    void error();

    bool isFailed() const
    {
        return hasErrors;
    }

protected:
    Settings *const &settings = settingsValue;
    BuildHistory *const &bh = bhValue;
    Repository *const &repo = repoValue;

private:
    std::string makeExpectedMsg() const;
    bool isAlias(const std::string &alias) const;

private:
    virtual void execImpl(const std::string &alias,
                          const std::vector<std::string> &args) = 0;

private:
    const std::vector<std::string> names;
    const std::size_t minArgs;
    const std::size_t maxArgs;

    int hasErrors = false;
    std::unordered_map<std::string, std::string> descriptions;
    Settings *settingsValue;
    BuildHistory *bhValue;
    Repository *repoValue;
};

/**
 * @brief Helper class that auto-registers its derivative in Commands-list.
 *
 * @tparam C Derived class (as per CRTP).
 */
template <class C>
class AutoSubCommand : public SubCommand
{
public:
    /**
     * @brief Pull in parent constructor.
     */
    using SubCommand::SubCommand;

private:
    /**
     * @brief Static initialization of this variable performs the registration.
     */
    static const bool invokeRegister;

private:
    /**
     * @brief Purpose of this field it to make @c invokeRegister used.
     */
    const bool forceRegistration = invokeRegister;
};

template <class C>
const bool AutoSubCommand<C>::invokeRegister = []() {
    registerCmd<C>();
    return true;
}();

#endif // UNCOV__SUBCOMMAND_HPP__
