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

#include "Invocation.hpp"

#include <boost/program_options.hpp>

#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace po = boost::program_options;

static po::variables_map parseOptions(const std::vector<std::string> &args);
static std::vector<po::option>
stopAtFirstPositional(std::vector<std::string> &args);

Invocation::Invocation(std::vector<std::string> args)
{
    if (args.empty()) {
        throw std::invalid_argument("Broken argument list.");
    }

    // Extract program name.
    programName = args[0];
    args.erase(args.cbegin());

    if (args.empty()) {
        error = "No arguments.";
        return;
    }

    po::variables_map varMap;
    try {
        varMap = parseOptions(args);
    } catch (const std::exception &e) {
        error = e.what();
        return;
    }

    printHelp = varMap.count("help");
    printVersion = varMap.count("version");
    args = varMap["positional"].as<std::vector<std::string>>();

    if (printHelp || printVersion) {
        return;
    }

    if (!args.empty()) {
        auto isPath = [](const std::string &s) {
            return s.substr(0, 1) == "." || s.find('/') != std::string::npos;
        };

        // Extract path to repository.
        if (isPath(args.front())) {
            repositoryPath = args.front();
            args.erase(args.cbegin());
        } else {
            repositoryPath = ".";
        }
    }

    if (args.empty()) {
        error = "No subcommand specified.";
        return;
    }

    // Extract subcommand and its arguments.
    subcommandName = args.front();
    args.erase(args.cbegin());
    subcommandArgs = std::move(args);
}

/**
 * @brief Parses command line-options.
 *
 * Positional arguments are returned in "positional" entry, which exists even
 * when there is no positional arguments.
 *
 * @param args Command-line arguments.
 *
 * @returns Variables map of option values.
 */
static po::variables_map
parseOptions(const std::vector<std::string> &args)
{
    po::options_description hiddenOpts;
    hiddenOpts.add_options()
        ("positional", po::value<std::vector<std::string>>()
                       ->default_value({}, ""),
         "positional args");

    po::positional_options_description positionalOptions;
    positionalOptions.add("positional", -1);

    po::options_description cmdlineOptions;

    cmdlineOptions.add_options()
        ("help,h", "display help message")
        ("version,v", "display version");

    po::options_description allOptions;
    allOptions.add(cmdlineOptions).add(hiddenOpts);

    auto parsed_from_cmdline =
        po::command_line_parser(args)
        .options(allOptions)
        .positional(positionalOptions)
        .extra_style_parser(&stopAtFirstPositional)
        .run();

    po::variables_map varMap;
    po::store(parsed_from_cmdline, varMap);
    return varMap;
}

/**
 * @brief Command-line option parser that captures as positional argument any
 *        element starting from the first positional argument.
 *
 * @param args Arguments.
 *
 * @returns Parsed arguments.
 */
static std::vector<po::option>
stopAtFirstPositional(std::vector<std::string> &args)
{
    std::vector<po::option> result;
    const std::string &tok = args[0];
    if (!tok.empty() && tok.front() != '-') {
        for (unsigned int i = 0U; i < args.size(); ++i) {
            po::option opt;
            opt.value.push_back(args[i]);
            opt.original_tokens.push_back(args[i]);
            opt.position_key = std::numeric_limits<int>::max();
            result.push_back(opt);
        }
        args.clear();
    }
    return result;
}

std::string
Invocation::getUsage() const
{
    return "Usage: " + programName
         + " [--help|-h] [--version|-v] [repo] subcommand [args...]";
}

const std::string &
Invocation::getError() const
{
    return error;
}

const std::string &
Invocation::getRepositoryPath() const
{
    return repositoryPath;
}

const std::string &
Invocation::getSubcommandName() const
{
    return subcommandName;
}

const std::vector<std::string> &
Invocation::getSubcommandArgs() const
{
    return subcommandArgs;
}

bool
Invocation::shouldPrintHelp() const
{
    return printHelp;
}

bool
Invocation::shouldPrintVersion() const
{
    return printVersion;
}
