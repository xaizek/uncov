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

#ifndef UNCOVER__ARG_PARSING_HPP__
#define UNCOVER__ARG_PARSING_HPP__

#include <boost/optional/optional_fwd.hpp>

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

struct Nothing {};

const int LatestBuildMarker = 0;

namespace detail
{

// Map of input tags to output types.
template <typename InType>
struct ToOutType;
template <>
struct ToOutType<BuildId> { using type = int; };
template <>
struct ToOutType<FilePath> { using type = std::string; };
template <>
struct ToOutType<PositiveNumber> { using type = unsigned int; };
template <typename T>
struct ToOutType<StringLiteral<T>> { using type = Nothing; };

enum class ParseResult
{
    Accepted,
    Rejected,
    Skipped
};

template <typename T>
struct parseArg;

template <>
struct parseArg<BuildId>
{
    static std::pair<typename ToOutType<BuildId>::type, ParseResult>
    parse(const std::vector<std::string> &args, std::size_t idx);
};

template <>
struct parseArg<FilePath>
{
    static std::pair<typename ToOutType<FilePath>::type, ParseResult>
    parse(const std::vector<std::string> &args, std::size_t idx);
};

template <>
struct parseArg<PositiveNumber>
{
    static std::pair<typename ToOutType<PositiveNumber>::type, ParseResult>
    parse(const std::vector<std::string> &args, std::size_t idx);
};

template <typename T>
struct parseArg<StringLiteral<T>>
{
    static std::pair<typename ToOutType<StringLiteral<T>>::type, ParseResult>
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
boost::optional<std::tuple<typename ToOutType<T>::type>>
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
boost::optional<std::tuple<typename ToOutType<T1>::type,
                           typename ToOutType<T2>::type,
                           typename ToOutType<Types>::type...>>
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
boost::optional<std::tuple<typename detail::ToOutType<Types>::type...>>
tryParse(const std::vector<std::string> &args)
{
    return detail::tryParse<Types...>(args, 0U);
}

#endif // UNCOVER__ARG_PARSING_HPP__
