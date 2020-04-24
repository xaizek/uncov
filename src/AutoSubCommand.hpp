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

#ifndef UNCOV__AUTOSUBCOMMAND_HPP__
#define UNCOV__AUTOSUBCOMMAND_HPP__

#include <functional>
#include <iostream>
#include <string>

#include "SubCommand.hpp"
#include "arg_parsing.hpp"

/**
 * @file AutoSubCommand.hpp
 *
 * @brief This unit makes implementing sub-commands easier.
 */

namespace detail
{

/**
 * @brief Prints placeholder for BuildId type of parameters.
 *
 * @param os    Output stream.
 * @param param Marker for dispatching on type.
 */
inline void
printParam(std::ostream &os, Lst<BuildId> /*param*/)
{ os << "<build>"; }

/**
 * @brief Prints placeholder for OptBuildId type of parameters.
 *
 * @param os    Output stream.
 * @param param Marker for dispatching on type.
 */
inline void
printParam(std::ostream &os, Lst<OptBuildId> /*param*/)
{ os << "[<build>]"; }

/**
 * @brief Prints placeholder for String type of parameters.
 *
 * @tparam T Provider of string literal's placeholder (`placeholder` field).
 *
 * @param os    Output stream.
 * @param param Marker for dispatching on type.
 */
template <typename T>
void
printParam(std::ostream &os, Lst<String<T>> /*param*/)
{ os << T::placeholder; }

/**
 * @brief Prints placeholder for PositiveNumber type of parameters.
 *
 * @param os    Output stream.
 * @param param Marker for dispatching on type.
 */
inline void
printParam(std::ostream &os, Lst<PositiveNumber> /*param*/)
{ os << "<positive-num>"; }

/**
 * @brief Prints placeholder for StringLiteral type of parameters.
 *
 * @tparam T Provider of string literal's value.
 *
 * @param os    Output stream.
 * @param param Marker for dispatching on type.
 */
template <typename T>
void
printParam(std::ostream &os, Lst<StringLiteral<T>> /*param*/)
{ os << '"' << T::text << '"'; }

/**
 * @brief Catch all form of printParam to get better error messages.
 *
 * @param param Marker for dispatching on type.
 */
inline void
printParam(...) = delete;

/**
 * @brief Prints empty list of parameters.
 *
 * @param os     Output stream.
 * @param params List of parameters.
 */
inline void
printParams(std::ostream &os, Lst<> /*params*/)
{ os << '\n'; }

/**
 * @brief Prints placeholder for list consisting of a single parameter.
 *
 * @tparam T Type of the parameter.
 *
 * @param os     Output stream.
 * @param params List of parameters.
 */
template <typename T>
void
printParams(std::ostream &os, Lst<T> /*params*/)
{
    printParam(os, Lst<T>{});
    os << '\n';
}

/**
 * @brief Prints placeholders for list consisting of multiple parameters.
 *
 * @tparam T     Type of the first parameter.
 * @tparam Types Types of the rest of the parameters.
 *
 * @param os     Output stream.
 * @param params List of parameters.
 */
template <typename T, typename... Types>
void
printParams(std::ostream &os, Lst<T, Types...> /*params*/)
{
    printParam(os, Lst<T>{});
    if (sizeof...(Types) != 0) {
        os << ' ';
    }
    printParams(os, Lst<Types...>{});
}

/**
 * @brief Prints command invocation based on list of parameters.
 *
 * @tparam Types Types of parameters.
 *
 * @param os     Output stream.
 * @param alias  Alias of the command.
 * @param params List of parameters.
 */
template <typename... Types>
void
printInvocation(std::ostream &os, const std::string &alias,
                Lst<Types...> params)
{
    os << " * uncov " << alias;
    if (sizeof...(Types) != 0) {
        os << ' ';
    }
    printParams(os, params);
}

/**
 * @brief Prints invocation information for a command.
 *
 * @tparam Lists List of invocations (of type `Lst<...>`).
 *
 * @param os    Output stream.
 * @param alias Alias of the command.
 * @param lists Invocation forms.
 */
template <typename... Lists>
void
printHelpMsg(std::ostream &os, const std::string &alias,
             Lst<Lists...> /*lists*/)
{
    static_assert(sizeof...(Lists) > 0,
                  "There must be at least one invocation form.");

    os << "Valid invocation forms:\n";
    auto initList = { (printInvocation(os, alias, Lists{}), false)... };
    (void)initList;
}

}

/**
 * @brief Helper class that auto-registers its derivative in Commands-list.
 *
 * @tparam C Derived class (as per CRTP).
 *
 * Derived class must have `callForms` typedef that is a non-empty `Lst` of
 * `Lst`s.
 */
template <class C>
class AutoSubCommand : public SubCommand
{
public:
    // Pull in parent constructor.
    using SubCommand::SubCommand;

    /**
     * @brief Signals that an invocation error has occurred.
     *
     * @param alias Alias of the command.
     */
    void usageError(const std::string &alias);

    virtual void printHelp(std::ostream &os,
                           const std::string &alias) const override;

private:
    //! Static initialization of this variable performs the registration.
    static const bool invokeRegister;

private:
    //! Purpose of this field it to make @c invokeRegister used.
    const bool forceRegistration = invokeRegister;
};

template <class C>
const bool AutoSubCommand<C>::invokeRegister = (registerCmd<C>(), true);

template <class C>
void
AutoSubCommand<C>::usageError(const std::string &alias)
{
    std::cerr << "Failed to parse arguments for `" << alias << "`.\n";
    printHelp(std::cerr, alias);
    error();
}

template <class C>
void
AutoSubCommand<C>::printHelp(std::ostream &os, const std::string &alias) const
{
    detail::printHelpMsg(os, alias, typename C::callForms{});
}

#endif // UNCOV__AUTOSUBCOMMAND_HPP__
