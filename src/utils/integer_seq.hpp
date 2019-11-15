// Copyright (C) 2017 xaizek <xaizek@posteo.net>
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

#ifndef UNCOV__UTILS__INTEGER_SEQ_HPP__
#define UNCOV__UTILS__INTEGER_SEQ_HPP__

#include <cstddef>

/**
 * @file integer_seq.hpp
 *
 * @brief This unit implements integer sequence from C++14 standard.
 */

/**
 * @brief This type provides storage for integer sequence in its parameters.
 *
 * @tparam Idxs Indexes this sequence contains.
 */
template <std::size_t... Idxs>
struct integer_sequence {};

/**
 * @brief Builder of integer sequence.
 *
 * @tparam Ts Any types.
 */
template <typename... Ts>
struct Idx;

/**
 * @brief Tail specialization of integer sequence builder.
 *
 * @tparam T Tail type.
 */
template <typename T>
struct Idx<T>
{
    //! Integer sequence type.
    using type = integer_sequence<0U>;
};

/**
 * @brief Helper template for extending integer sequence by one element.
 *
 * @tparam T Any type.
 */
template <typename T>
struct Extend;

/**
 * @brief Extends integer sequence by one element.
 *
 * @tparam Is Integer sequence.
 */
template <std::size_t... Is>
struct Extend<integer_sequence<Is...>>
{
    //! Integer sequence type that is longer by one element.
    using type = integer_sequence<Is..., sizeof...(Is)>;
};

/**
 * @brief Prefix specialization of integer sequence builder.
 *
 * @tparam T Head type.
 * @tparam Ts Tail types.
 */
template <typename T, typename... Ts>
struct Idx<T, Ts...>
{
    //! Integer sequence type.
    using type = typename Extend<typename Idx<Ts...>::type>::type;
};

/**
 * @brief Convenience typedef for building integer sequence.
 *
 * @tparam Ts Types.
 */
template <typename... Ts>
using index_sequence_for = typename Idx<Ts...>::type;

#endif // UNCOV__UTILS__INTEGER_SEQ_HPP__
