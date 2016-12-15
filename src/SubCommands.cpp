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

#include <sys/ioctl.h>
#include <sys/wait.h>
#include <termios.h>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream_buffer.hpp>
#include <boost/optional.hpp>
#include <boost/scope_exit.hpp>

#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "BuildHistory.hpp"
#include "FilePrinter.hpp"
#include "Repository.hpp"
#include "SubCommand.hpp"
#include "TablePrinter.hpp"
#include "coverage.hpp"
#include "decoration.hpp"

static void printBuildHeader(BuildHistory *bh, const Build &build);
static void printFile(const Repository *repo, const Build &build,
                      const File &file, FilePrinter &printer);
static void printFileHeader(const File &file);
static void printLineSeparator();

class RedirectToPager
{
    template <typename T>
    using stream_buffer = boost::iostreams::stream_buffer<T>;
    using file_descriptor_sink = boost::iostreams::file_descriptor_sink;

public:
    RedirectToPager()
    {
        int pipePair[2];
        if (pipe(pipePair) != 0) {
            throw std::runtime_error("Failed to create a pipe");
        }
        BOOST_SCOPE_EXIT_ALL(pipePair) { close(pipePair[0]); };

        pid = fork();
        if (pid == -1) {
            close(pipePair[1]);
            throw std::runtime_error("Fork has failed");
        }
        if (pid == 0) {
            close(pipePair[1]);
            if (dup2(pipePair[0], STDIN_FILENO) == -1) {
                _Exit(EXIT_FAILURE);
            }
            close(pipePair[0]);
            // XXX: hard-coded invocation of less.
            execlp("less", "less", "-R", static_cast<char *>(nullptr));
            _Exit(127);
        }

        out.open(file_descriptor_sink(pipePair[1],
                                      boost::iostreams::close_handle));
        rdbuf = std::cout.rdbuf(&out);

        // XXX: here we could add a custom stream buffer, which would collect up
        //      to <terminal height> lines and if buffer is closed with this
        //      limit not reached, put lines on the screen as is; if we hit the
        //      limit in the process, open a pipe and redirect everything we got
        //      and all new output there.
    }

    ~RedirectToPager()
    {
        std::cout.rdbuf(rdbuf);
        out.close();
        int wstatus;
        waitpid(pid, &wstatus, 0);
    }

private:
    stream_buffer<file_descriptor_sink> out;
    std::streambuf *rdbuf;
    pid_t pid;
};

/**
 * @brief Retrieves terminal width.
 *
 * @returns Actual terminal width, or maximum possible value for the type.
 */
static unsigned int
getTerminalWidth()
{
    winsize ws;
    if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) != 0) {
        return std::numeric_limits<unsigned int>::max();
    }

    return ws.ws_col;
}

namespace
{

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
    parse(const std::vector<std::string> &args, std::size_t idx)
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
        return { 0, ParseResult::Skipped };
    }
};

template <>
struct parseArg<FilePath>
{
    static std::pair<typename ToOutType<FilePath>::type, ParseResult>
    parse(const std::vector<std::string> &args, std::size_t idx)
    {
        if (idx < args.size()) {
            return { args[idx], ParseResult::Accepted };
        }
        return { {}, ParseResult::Rejected };
    }
};

template <>
struct parseArg<PositiveNumber>
{
    static std::pair<typename ToOutType<PositiveNumber>::type, ParseResult>
    parse(const std::vector<std::string> &args, std::size_t idx)
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

namespace CmdUtils
{
    Build
    getBuild(BuildHistory *bh, int buildId)
    {
        if (buildId == LatestBuildMarker) {
            buildId = bh->getLastBuildId();
            if (buildId == 0) {
                throw std::runtime_error("No last build");
            }
        }

        boost::optional<Build> build = bh->getBuild(buildId);
        if (!build) {
            throw std::runtime_error {
                "Can't find build #" + std::to_string(buildId)
            };
        }
        return *build;
    }

    File &
    getFile(const Build &build, const std::string &path)
    {
        boost::optional<File &> file = build.getFile(path);
        if (!file) {
            throw std::runtime_error("Can't find file: " + path +
                                     " in build #" +
                                     std::to_string(build.getId()) + " of " +
                                     build.getRefName() + " at " +
                                     build.getRef());
        }
        return *file;
    }
}

}

class BuildsCmd : public AutoSubCommand<BuildsCmd>
{
    struct All { static constexpr const char *const text = "all"; };

public:
    BuildsCmd() : parent("builds", 0U, 1U)
    {
    }

private:
    virtual void
    execImpl(const std::vector<std::string> &args) override
    {
        // By default limit number of builds to display to 10.
        bool limitBuilds = true;
        unsigned int maxBuildCount = 10;
        if (auto parsed = tryParse<PositiveNumber>(args)) {
            std::tie(maxBuildCount) = *parsed;
        } else if (tryParse<StringLiteral<All>>(args)) {
            limitBuilds = false;
        }

        // TODO: colorize percents?
        TablePrinter tablePrinter {
            { "Build", "Coverage", "C/R Lines", "Coverage Change",
              "C/U/R Line Changes", "Branch", "Commit" },
            getTerminalWidth()
        };

        std::vector<Build> builds = bh->getBuilds();
        if (limitBuilds && builds.size() > maxBuildCount) {
            builds.erase(builds.cbegin(), builds.cend() - maxBuildCount);
        }

        std::string sharp("#"), percent(" %"), sep(" / ");
        for (const Build &build : builds) {
            CovInfo covInfo(build);

            const int prevBuildId = bh->getPreviousBuildId(build.getId());
            CovInfo prevCovInfo;
            if (prevBuildId != 0) {
                prevCovInfo = CovInfo(*bh->getBuild(prevBuildId));
            }

            CovChange covChange(prevCovInfo, covInfo);

            tablePrinter.append({
                sharp + std::to_string(build.getId()),
                covInfo.formatCoverageRate() + percent,
                covInfo.formatLines(sep),
                covChange.formatCoverageRate() + percent,
                covChange.formatLines(sep),
                build.getRefName(),
                build.getRef()
            });
        }

        tablePrinter.print(std::cout);
    }
};

class DiffCmd : public AutoSubCommand<DiffCmd>
{
public:
    DiffCmd() : parent("diff", 2U, 3U)
    {
    }

private:
    virtual void
    execImpl(const std::vector<std::string> &args) override
    {
        // TODO: allow diffing whole builds.

        int oldBuildId, newBuildId;
        std::string filePath;
        if (auto parsed = tryParse<BuildId, BuildId, FilePath>(args)) {
            std::tie(oldBuildId, newBuildId, filePath) = *parsed;
        } else {
            std::cerr << "Invalid arguments for subcommand.\n";
            return error();
        }

        Build oldBuild = CmdUtils::getBuild(bh, oldBuildId);
        Build newBuild = CmdUtils::getBuild(bh, newBuildId);

        File &oldFile = CmdUtils::getFile(oldBuild, filePath);
        File &newFile = CmdUtils::getFile(newBuild, filePath);

        Text oldVersion = repo->readFile(oldBuild.getRef(),
                                         oldFile.getPath());
        Text newVersion = repo->readFile(newBuild.getRef(),
                                         newFile.getPath());
        const std::vector<int> &oldCov = oldFile.getCoverage();
        const std::vector<int> &newCov = newFile.getCoverage();

        if (oldVersion.size() != oldCov.size() ||
            newVersion.size() != newCov.size()) {
            std::cerr << "Coverage information is not accurate\n";
            return error();
        }

        RedirectToPager redirectToPager;

        printLineSeparator();
        printBuildHeader(bh, oldBuild);
        printFileHeader(oldFile);
        printLineSeparator();
        printBuildHeader(bh, newBuild);
        printFileHeader(newFile);
        printLineSeparator();

        FilePrinter filePrinter;
        filePrinter.printDiff(args[2], oldVersion, oldCov, newVersion, newCov);

        // TODO: print some totals/stats here.
    }
};

class DirsCmd : public AutoSubCommand<DirsCmd>
{
public:
    DirsCmd() : parent("dirs", 0U, 1U)
    {
    }

private:
    virtual void
    execImpl(const std::vector<std::string> &args) override
    {
        namespace fs = boost::filesystem;

        int buildId;
        if (auto parsed = tryParse<BuildId>(args)) {
            buildId = std::get<0>(*parsed);
        } else {
            std::cerr << "Invalid arguments for subcommand.\n";
            return error();
        }

        Build build = CmdUtils::getBuild(bh, buildId);

        std::map<std::string, CovInfo> dirs;
        for (const std::string &filePath : build.getPaths()) {
            fs::path dirPath = filePath;
            dirPath.remove_filename();

            File &file = *build.getFile(filePath);
            CovInfo &info = dirs[dirPath.string()];

            info.add(CovInfo(file));
        }

        // TODO: colorize percents?
        TablePrinter tablePrinter({ "-Directory", "Coverage", "#" },
                                  getTerminalWidth());

        std::string slash("/"), percent(" %"), sep(" / ");
        for (const auto &entry : dirs) {
            const CovInfo &covInfo = entry.second;
            tablePrinter.append({
                entry.first + slash,
                covInfo.formatCoverageRate() + percent,
                covInfo.formatLines(sep)
            });
        }

        tablePrinter.print(std::cout);
    }
};

class FilesCmd : public AutoSubCommand<FilesCmd>
{
public:
    FilesCmd() : parent("files", 0U, 1U)
    {
    }

private:
    virtual void
    execImpl(const std::vector<std::string> &args) override
    {
        int buildId;
        if (auto parsed = tryParse<BuildId>(args)) {
            buildId = std::get<0>(*parsed);
        } else {
            std::cerr << "Invalid arguments for subcommand.\n";
            return error();
        }

        Build build = CmdUtils::getBuild(bh, buildId);

        // TODO: colorize percents?
        TablePrinter tablePrinter({ "-File", "Coverage", "#" },
                                  getTerminalWidth());

        std::string percent(" %"), sep(" / ");
        for (const std::string &filePath : build.getPaths()) {
            CovInfo covInfo(*build.getFile(filePath));
            tablePrinter.append({
                filePath,
                covInfo.formatCoverageRate() + percent,
                covInfo.formatLines(sep)
            });
        }

        tablePrinter.print(std::cout);
    }
};

class GetCmd : public AutoSubCommand<GetCmd>
{
public:
    GetCmd() : parent("get", 2U)
    {
    }

private:
    virtual void
    execImpl(const std::vector<std::string> &args) override
    {
        int buildId;
        std::string filePath;
        if (auto parsed = tryParse<BuildId, FilePath>(args)) {
            std::tie(buildId, filePath) = *parsed;
        } else {
            std::cerr << "Invalid arguments for subcommand.\n";
            return error();
        }

        Build build = CmdUtils::getBuild(bh, buildId);
        File &file = CmdUtils::getFile(build, filePath);

        std::cout << build.getRef() << '\n';
        for (int hits : file.getCoverage()) {
            std::cout << hits << '\n';
        }
    }
};

class NewCmd : public AutoSubCommand<NewCmd>
{
public:
    NewCmd() : parent("new")
    {
    }

private:
    virtual void
    execImpl(const std::vector<std::string> &/*args*/) override
    {
        std::string ref, refName;
        std::cin >> ref >> refName;

        const std::unordered_map<std::string, std::string> files =
            repo->listFiles(ref);

        BuildData bd(std::move(ref), refName);

        for (std::string path, sha1Hash; std::cin >> path >> sha1Hash; ) {
            int nLines;
            std::cin >> nLines;

            std::vector<int> coverage;
            coverage.reserve(nLines);

            while (nLines-- > 0) {
                int i;
                std::cin >> i;
                coverage.push_back(i);
            }

            const auto file = files.find(path);
            if (file == files.cend()) {
                std::cerr << "Skipping file missing in " << refName << ": "
                          << path << '\n';
            } else if (!boost::iequals(file->second, sha1Hash)) {
                std::cerr << path << " file at " << refName
                          << " doesn't match reported SHA-1\n";
                error();
            } else {
                bd.addFile(File(std::move(path), std::move(coverage)));
            }
        }

        if (!isFailed()) {
            Build build = bh->addBuild(bd);
            printBuildHeader(bh, build);
        }
    }
};

class ShowCmd : public AutoSubCommand<ShowCmd>
{
public:
    ShowCmd() : parent("show", 0U, 2U)
    {
    }

private:
    virtual void
    execImpl(const std::vector<std::string> &args) override
    {
        // TODO: maybe allow passing in path to directory, which would cause
        //       printing all files in that sub-tree (or only directly in it?).

        int buildId;
        std::string filePath;
        bool printWholeBuild = false;
        if (auto parsed = tryParse<BuildId>(args)) {
            buildId = std::get<0>(*parsed);
            printWholeBuild = true;
        } else if (auto parsed = tryParse<FilePath>(args)) {
            buildId = 0;
            filePath = std::get<0>(*parsed);
        } else if (auto parsed = tryParse<BuildId, FilePath>(args)) {
            std::tie(buildId, filePath) = *parsed;
        } else {
            std::cerr << "Invalid arguments for subcommand.\n";
            return error();
        }

        Build build = CmdUtils::getBuild(bh, buildId);
        FilePrinter printer;

        if (printWholeBuild) {
            RedirectToPager redirectToPager;
            printBuildHeader(bh, build);
            for (const std::string &path : build.getPaths()) {
                printFile(repo, build, *build.getFile(path), printer);
            }
        } else {
            File &file = CmdUtils::getFile(build, filePath);
            RedirectToPager redirectToPager;
            printBuildHeader(bh, build);
            printFile(repo, build, file, printer);
        }
    }
};

static void
printBuildHeader(BuildHistory *bh, const Build &build)
{
    CovInfo covInfo(build);

    const int prevBuildId = bh->getPreviousBuildId(build.getId());
    CovInfo prevCovInfo;
    if (prevBuildId != 0) {
        prevCovInfo = CovInfo(*bh->getBuild(prevBuildId));
    }

    CovChange covChange(prevCovInfo, covInfo);

    // TODO: colorize percents?
    std::cout << (decor::bold << "Build:") << " #" << build.getId() << ", "
              << covInfo.formatCoverageRate() << "% "
              << '(' << covInfo.formatLines("/") << "), "
              << covChange.formatCoverageRate() << "% "
              << '(' << covChange.formatLines("/") << "), "
              << build.getRefName() << " at " << build.getRef() << '\n';
}

static void
printFile(const Repository *repo, const Build &build, const File &file,
          FilePrinter &printer)
{
    printLineSeparator();
    printFileHeader(file);
    printLineSeparator();

    const std::string &path = file.getPath();
    const std::string &ref = build.getRef();
    printer.print(path, repo->readFile(ref, path), file.getCoverage());
}

static void
printFileHeader(const File &file)
{
    CovInfo covInfo(file);
    // TODO: colorize percents?
    std::cout << (decor::bold << "File: ") << file.getPath() << ", "
              << covInfo.formatCoverageRate() << "% "
              << '(' << covInfo.formatLines("/") << ")\n";
}

static void
printLineSeparator()
{
    std::cout << std::setfill('-') << std::setw(80) << '\n'
              << std::setfill(' ');
}
