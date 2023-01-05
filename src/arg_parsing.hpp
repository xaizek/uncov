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

#ifndef UNCOV_ARG_PARSING_HPP_
#define UNCOV_ARG_PARSING_HPP_

#include <boost/optional.hpp>
#include <boost/variant/variant_fwd.hpp>

#include <cstddef>

#include <functional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

/**
 * @file arg_parsing.hpp
 *
 * @brief Positional arguments parsing facility.
 */

//! Build id parameter (not defined).
struct BuildId;
//! Optional build id parameter (not defined).
struct OptBuildId;
//! Any string parameter (not defined).  `T` can be used to provide extra data.
template <typename T>
struct String;
//! Positive number parameter (not defined).
struct PositiveNumber;
//! Overload resolution wrapper for string literal classes (not defined).
template <typename T>
struct StringLiteral;

//! Special output type.
struct Nothing {};

//! Default build id when not provided in input arguments.
const int LatestBuildMarker = 0;

namespace detail
{

/**
 * @brief Parser which must be specialized per parameter type.
 *
 * @tparam T Input type.
 */
template <typename T>
struct parseArg;

/**
 * @brief Maps input tag to output type.
 *
 * @tparam InType Input tag.
 */
template <typename InType>
using ToOutType = typename parseArg<InType>::resultType;

/**
 * @brief Single argument parsing result for internal machinery.
 */
enum class ParseResult
{
    Accepted, //!< Matcher accepted input and parsing can continue.
    Rejected, //!< Matcher failed to match input, which is fatal error.
    Skipped   //!< Matcher matches zero arguments and parsing can go on.
};

/**
 * @brief Parses build id, which is of the form `/@@|@number/`.
 */
template <>
struct parseArg<BuildId>
{
    //! Type of result yielded by this parser.
    using resultType = boost::variant<int, std::string>;

    /**
     * @brief Parses build id.
     *
     * @param args Input arguments.
     * @param idx  Current position within @p args or just past its end.
     *
     * @returns Parsed value and indication whether parsing was successful.
     */
    static std::pair<resultType, ParseResult>
    parse(const std::vector<std::string> &args, std::size_t idx);
};

/**
 * @brief Parses optional build id, which is of the form `/@@|@number/`.
 */
template <>
struct parseArg<OptBuildId>
{
    //! Type of result yielded by this parser.
    using resultType = boost::variant<int, std::string>;

    /**
     * @brief Parses optional build id.
     *
     * @param args Input arguments.
     * @param idx  Current position within @p args or just past its end.
     *
     * @returns Parsed value and indication whether parsing was successful.
     */
    static std::pair<resultType, ParseResult>
    parse(const std::vector<std::string> &args, std::size_t idx);
};

/**
 * @brief Acccepts any string.
 *
 * @tparam T Arbitrary type.
 */
template <typename T>
struct parseArg<String<T>>
{
    //! Type of result yielded by this parser.
    using resultType = std::string;

    /**
     * @brief Parses build id.
     *
     * @param args Input arguments.
     * @param idx  Current position within @p args or just past its end.
     *
     * @returns Parsed value and indication whether parsing was successful.
     */
    static std::pair<resultType, ParseResult>
    parse(const std::vector<std::string> &args, std::size_t idx)
    {
        if (idx < args.size()) {
            return { args[idx], ParseResult::Accepted };
        }
        return { {}, ParseResult::Rejected };
    }
};

/**
 * @brief Parses an argument that is a positive number (> 0).
 */
template <>
struct parseArg<PositiveNumber>
{
    //! Type of result yielded by this parser.
    using resultType = unsigned int;

    /**
     * @brief Parses build id.
     *
     * @param args Input arguments.
     * @param idx  Current position within @p args or just past its end.
     *
     * @returns Parsed value and indication whether parsing was successful.
     */
    static std::pair<resultType, ParseResult>
    parse(const std::vector<std::string> &args, std::size_t idx);
};

/**
 * @brief Parses an argument that strictly matches string literal.
 *
 * @tparam T Arbitrary type that provides @c text field with the literal.
 */
template <typename T>
struct parseArg<StringLiteral<T>>
{
    //! Type of result yielded by this parser.
    using resultType = Nothing;

    /**
     * @brief Parses build id.
     *
     * @param args Input arguments.
     * @param idx  Current position within @p args or just past its end.
     *
     * @returns Parsed value and indication whether parsing was successful.
     */
    static std::pair<resultType, ParseResult>
    parse(const std::vector<std::string> &args, std::size_t idx)
    {
        if (idx >= args.size()) {
            return { {}, ParseResult::Rejected };
        }
        if (args[idx] == T::text) {
            return { {}, ParseResult::Accepted };
        }
        return { {}, ParseResult::Rejected };
    }
};

/**
 * @brief Dispatches single-parameter format.
 *
 * @tparam T Type of the first expected parameter.
 *
 * @param args Input arguments.
 * @param idx  Current position within @p args or just past its end.
 *
 * @returns Unit tuple with result of parsing if it was successful.
 */
template <typename T>
boost::optional<std::tuple<ToOutType<T>>>
tryParse(const std::vector<std::string> &args, std::size_t idx)
{
    auto parsed = parseArg<T>::parse(args, idx);
    if (parsed.second == ParseResult::Accepted) {
        if (idx == args.size() - 1U) {
            return std::make_tuple(parsed.first);
        }
    } else if (parsed.second == ParseResult::Skipped) {
        if (idx >= args.size()) {
            return std::make_tuple(parsed.first);
        }
    }
    return {};
}

/**
 * @brief Dispatches multiple-parameter format.
 *
 * @tparam T1    Type of the first expected parameter.
 * @tparam T2    Type of the second expected parameter.
 * @tparam Types Types of the rest expected parameters (might be empty).
 *
 * @param args Input arguments.
 * @param idx  Current position within @p args or just past its end.
 *
 * @returns Tuple of results of parsing if it was successful.
 */
template <typename T1, typename T2, typename... Types>
boost::optional<std::tuple<ToOutType<T1>, ToOutType<T2>, ToOutType<Types>...>>
tryParse(const std::vector<std::string> &args, std::size_t idx)
{
    auto parsed = parseArg<T1>::parse(args, idx);
    switch (parsed.second) {
        case ParseResult::Accepted:
            if (auto tail = tryParse<T2, Types...>(args, idx + 1)) {
                return std::tuple_cat(std::make_tuple(parsed.first), *tail);
            }
            break;
        case ParseResult::Rejected:
            return {};
        case ParseResult::Skipped:
            if (auto tail = tryParse<T2, Types...>(args, idx)) {
                return std::tuple_cat(std::make_tuple(parsed.first), *tail);
            }
            break;
    }
    return {};
}

/**
 * @brief Handles case of empty parameter list.
 *
 * @tparam Types Types of expected parameters (might be empty).
 *
 * @param args Input arguments.
 * @param idx  Current position within @p args or just past its end.
 *
 * @returns Empty tuple in case of successful parsing.
 */
template <typename... Types>
boost::optional<std::tuple<>>
tryParse(const std::vector<std::string> &args,
         typename std::enable_if<sizeof...(Types) == 0, std::size_t>::type idx)
{
    if (args.size() == 0 && idx == 0) {
        return std::make_tuple();
    }
    return {};
}

}

/**
 * @brief Captures list of types.
 *
 * @tparam Args List of types.
 */
template <typename... Types>
struct Lst
{ };

/**
 * @brief Parses argument list guided by format specified in template arguments.
 *
 * @tparam Types Types of expected parameters.
 *
 * @param args Input arguments.
 * @param form List of types of expected parameters.
 *
 * @returns Tuple of results of parsing if it was successful.
 */
template <typename... Types>
boost::optional<std::tuple<detail::ToOutType<Types>...>>
tryParse(const std::vector<std::string> &args, Lst<Types...> /*form*/)
{
    return detail::tryParse<Types...>(args, 0U);
}

#endif // UNCOV_ARG_PARSING_HPP_
