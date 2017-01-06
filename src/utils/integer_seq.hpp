// Copyright (C) 2017 xaizek <xaizek@openmailbox.org>
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

#ifndef UNCOV__UTILS__INTEGER_SEQ_HPP__
#define UNCOV__UTILS__INTEGER_SEQ_HPP__

#include <cstddef>

template <std::size_t...>
struct integer_sequence {};

template <typename... T>
struct Idx;

template <typename T>
struct Idx<T>
{
    using type = integer_sequence<0U>;
};

template <typename T>
struct Extend;

template <std::size_t... Is>
struct Extend<integer_sequence<Is...>>
{
    using type = integer_sequence<0U, (Is + 1U)...>;
};

template <typename T, typename... Ts>
struct Idx<T, Ts...>
{
    using type = typename Extend<typename Idx<Ts...>::type>::type;
};

template <typename... T>
using index_sequence_for = typename Idx<T...>::type;

#endif // UNCOV__UTILS__INTEGER_SEQ_HPP__
