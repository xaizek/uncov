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

#ifndef UNCOV__ARG_PARSING_HPP__
#define UNCOV__ARG_PARSING_HPP__

#include <boost/optional.hpp>

#include <cstddef>

#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

// Input tags for tryParse().  Not defined.
struct BuildId;
struct FilePath;
struct PositiveNumber;
template <typename T>
struct StringLiteral;

// Special output type.
struct Nothing {};

// Build id when not provided in arguments.
const int LatestBuildMarker = 0;

namespace detail
{

template <typename T>
struct parseArg;

// Map of input tags to output types.
template <typename InType>
using ToOutType = typename parseArg<InType>::resultType;

enum class ParseResult
{
    Accepted,
    Rejected,
    Skipped
};

template <>
struct parseArg<BuildId>
{
    using resultType = int;

    static std::pair<resultType, ParseResult>
    parse(const std::vector<std::string> &args, std::size_t idx);
};

template <>
struct parseArg<FilePath>
{
    using resultType = std::string;

    static std::pair<resultType, ParseResult>
    parse(const std::vector<std::string> &args, std::size_t idx);
};

template <>
struct parseArg<PositiveNumber>
{
    using resultType = unsigned int;

    static std::pair<resultType, ParseResult>
    parse(const std::vector<std::string> &args, std::size_t idx);
};

template <typename T>
struct parseArg<StringLiteral<T>>
{
    using resultType = Nothing;

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

}

template <typename... Types>
boost::optional<std::tuple<detail::ToOutType<Types>...>>
tryParse(const std::vector<std::string> &args)
{
    return detail::tryParse<Types...>(args, 0U);
}

#endif // UNCOV__ARG_PARSING_HPP__
