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

#include "arg_parsing.hpp"

#include <cstddef>

#include <string>
#include <utility>
#include <vector>

using namespace detail;

std::pair<typename ToOutType<BuildId>::type, ParseResult>
parseArg<BuildId>::parse(const std::vector<std::string> &args, std::size_t idx)
{
    if (idx < args.size()) {
        const std::string &arg = args[idx];
        if (arg == "@@") {
            return { LatestBuildMarker, ParseResult::Accepted };
        }
        if (arg.substr(0, 1) == "@") {
            try {
                std::size_t pos;
                const int i = std::stoi(arg.substr(1), &pos);
                if (pos == arg.length() - 1) {
                    return { i, ParseResult::Accepted };
                }
            } catch (const std::logic_error &) {
                throw std::runtime_error {
                    "Failed to parse subcommand argument #" +
                    std::to_string(idx + 1) + ": " + arg
                };
            }
        }
    }
    return { LatestBuildMarker, ParseResult::Skipped };
}

std::pair<typename ToOutType<FilePath>::type, ParseResult>
parseArg<FilePath>::parse(const std::vector<std::string> &args, std::size_t idx)
{
    if (idx < args.size()) {
        return { args[idx], ParseResult::Accepted };
    }
    return { {}, ParseResult::Rejected };
}

std::pair<typename ToOutType<PositiveNumber>::type, ParseResult>
parseArg<PositiveNumber>::parse(const std::vector<std::string> &args, std::size_t idx)
{
    if (idx >= args.size()) {
        return { 0, ParseResult::Rejected };
    }

    const std::string &arg = args[idx];
    try {
        std::size_t pos;
        const int i = std::stoul(arg, &pos);
        if (pos == arg.length()) {
            if (i <= 0) {
                throw std::runtime_error {
                    "Expected number greater than zero, got: " + arg
                };
            }
            return { i, ParseResult::Accepted };
        }
    } catch (const std::logic_error &) {
        return { {}, ParseResult::Rejected };
    }

    return { {}, ParseResult::Rejected };
}
