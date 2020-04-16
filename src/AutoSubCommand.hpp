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

#include "SubCommand.hpp"

/**
 * @brief Helper class that auto-registers its derivative in Commands-list.
 *
 * @tparam C Derived class (as per CRTP).
 */
template <class C>
class AutoSubCommand : public SubCommand
{
public:
    // Pull in parent constructor.
    using SubCommand::SubCommand;

private:
    //! Static initialization of this variable performs the registration.
    static const bool invokeRegister;

private:
    //! Purpose of this field it to make @c invokeRegister used.
    const bool forceRegistration = invokeRegister;
};

template <class C>
const bool AutoSubCommand<C>::invokeRegister = (registerCmd<C>(), true);

#endif // UNCOV__AUTOSUBCOMMAND_HPP__
