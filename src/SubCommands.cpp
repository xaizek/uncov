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
#include "arg_parsing.hpp"
#include "coverage.hpp"
#include "decoration.hpp"

static void printBuildHeader(BuildHistory *bh, const Build &build);
static CovChange getBuildCovChange(BuildHistory *bh, const Build &build,
                                   const CovInfo &covInfo);
static void printFile(BuildHistory *bh, const Repository *repo,
                      const Build &build, const File &file,
                      FilePrinter &printer);
static void printFileHeader(BuildHistory *bh, const Build &build,
                            const File &file);
static CovChange getFileCovChange(BuildHistory *bh, const Build &build,
                                  const std::string &path,
                                  boost::optional<Build> *prevBuildHint,
                                  const CovInfo &covInfo);
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

        TablePrinter tablePrinter {
            { "Build", "Coverage", "C/R Lines", "Cov Change",
              "C/U/R Line Changes", "Branch", "Commit" },
            getTerminalWidth()
        };

        std::vector<Build> builds = bh->getBuilds();
        if (limitBuilds && builds.size() > maxBuildCount) {
            builds.erase(builds.cbegin(), builds.cend() - maxBuildCount);
        }

        std::string sharp("#"), sep(" / ");
        for (const Build &build : builds) {
            CovInfo covInfo(build);
            CovChange covChange = getBuildCovChange(bh, build, covInfo);

            tablePrinter.append({
                sharp + std::to_string(build.getId()),
                covInfo.formatCoverageRate(),
                covInfo.formatLines(sep),
                covChange.formatCoverageRate(),
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
        printFileHeader(bh, oldBuild, oldFile);
        printLineSeparator();
        printBuildHeader(bh, newBuild);
        printFileHeader(bh, newBuild, newFile);
        printLineSeparator();

        FilePrinter filePrinter;
        filePrinter.printDiff(filePath, oldVersion, oldCov, newVersion, newCov);

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
        int buildId;
        if (auto parsed = tryParse<BuildId>(args)) {
            buildId = std::get<0>(*parsed);
        } else {
            std::cerr << "Invalid arguments for subcommand.\n";
            return error();
        }

        Build build = CmdUtils::getBuild(bh, buildId);

        std::map<std::string, CovInfo> newDirs = getDirsCoverage(build);

        std::map<std::string, CovInfo> prevDirs;
        if (const int prevBuildId = bh->getPreviousBuildId(build.getId())) {
            prevDirs = getDirsCoverage(*bh->getBuild(prevBuildId));
        }

        TablePrinter tablePrinter({ "-Directory", "Coverage", "C/R Lines",
                                    "Cov Change", "C/U/R Line Changes" },
                                  getTerminalWidth());

        std::string slash("/"), sep(" / ");
        for (const auto &entry : newDirs) {
            const CovInfo &covInfo = entry.second;
            CovChange covChange(prevDirs[entry.first], covInfo);

            tablePrinter.append({
                entry.first + slash,
                covInfo.formatCoverageRate(),
                covInfo.formatLines(sep),
                covChange.formatCoverageRate(),
                covChange.formatLines(sep)
            });
        }

        tablePrinter.print(std::cout);
    }

    std::map<std::string, CovInfo> getDirsCoverage(const Build &build) const
    {
        std::map<std::string, CovInfo> dirs;
        for (const std::string &filePath : build.getPaths()) {
            boost::filesystem::path dirPath = filePath;
            dirPath.remove_filename();

            File &file = *build.getFile(filePath);
            CovInfo &info = dirs[dirPath.string()];

            info.add(CovInfo(file));
        }
        return dirs;
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

        boost::optional<Build> prevBuild;
        if (const int prevBuildId = bh->getPreviousBuildId(build.getId())) {
            prevBuild = bh->getBuild(prevBuildId);
        }

        TablePrinter tablePrinter({ "-File", "Coverage", "C/R Lines",
                                    "Cov Change", "C/U/R Line Changes" },
                                  getTerminalWidth());

        std::string sep(" / ");
        for (const std::string &filePath : build.getPaths()) {
            CovInfo covInfo(*build.getFile(filePath));
            CovChange covChange = getFileCovChange(bh, build, filePath,
                                                   &prevBuild, covInfo);

            tablePrinter.append({
                filePath,
                covInfo.formatCoverageRate(),
                covInfo.formatLines(sep),
                covChange.formatCoverageRate(),
                covChange.formatLines(sep),
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
                printFile(bh, repo, build, *build.getFile(path), printer);
            }
        } else {
            File &file = CmdUtils::getFile(build, filePath);
            RedirectToPager redirectToPager;
            printBuildHeader(bh, build);
            printFile(bh, repo, build, file, printer);
        }
    }
};

static void
printBuildHeader(BuildHistory *bh, const Build &build)
{
    CovInfo covInfo(build);
    CovChange covChange = getBuildCovChange(bh, build, covInfo);

    std::cout << (decor::bold << "Build:") << " #" << build.getId() << ", "
              << covInfo.formatCoverageRate() << ' '
              << '(' << covInfo.formatLines("/") << "), "
              << covChange.formatCoverageRate() << ' '
              << '(' << covChange.formatLines("/") << "), "
              << build.getRefName() << " at " << build.getRef() << '\n';
}

static CovChange
getBuildCovChange(BuildHistory *bh, const Build &build, const CovInfo &covInfo)
{
    CovInfo prevCovInfo;
    if (const int prevBuildId = bh->getPreviousBuildId(build.getId())) {
        prevCovInfo = CovInfo(*bh->getBuild(prevBuildId));
    }

    return CovChange(prevCovInfo, covInfo);
}

static void
printFile(BuildHistory *bh, const Repository *repo, const Build &build,
          const File &file, FilePrinter &printer)
{
    printLineSeparator();
    printFileHeader(bh, build, file);
    printLineSeparator();

    const std::string &path = file.getPath();
    const std::string &ref = build.getRef();
    printer.print(path, repo->readFile(ref, path), file.getCoverage());
}

static void
printFileHeader(BuildHistory *bh, const Build &build, const File &file)
{
    CovInfo covInfo(file);
    CovChange covChange = getFileCovChange(bh, build, file.getPath(), nullptr,
                                           covInfo);

    std::cout << (decor::bold << "File: ") << file.getPath() << ", "
              << covInfo.formatCoverageRate() << ' '
              << '(' << covInfo.formatLines("/") << "), "
              << covChange.formatCoverageRate() << ' '
              << '(' << covChange.formatLines("/") << ")\n";
}

static CovChange
getFileCovChange(BuildHistory *bh, const Build &build, const std::string &path,
                 boost::optional<Build> *prevBuildHint, const CovInfo &covInfo)
{
    boost::optional<Build> prevBuild;
    if (prevBuildHint == nullptr) {
        if (const int prevBuildId = bh->getPreviousBuildId(build.getId())) {
            prevBuild = bh->getBuild(prevBuildId);
        }
        prevBuildHint = &prevBuild;
    }

    CovInfo prevCovInfo;
    if (*prevBuildHint) {
        boost::optional<File &> file = (*prevBuildHint)->getFile(path);
        if (file) {
            prevCovInfo = CovInfo(*file);
        }
    }

    return CovChange(prevCovInfo, covInfo);
}

static void
printLineSeparator()
{
    std::cout << std::setfill('-') << std::setw(80) << '\n'
              << std::setfill(' ');
}
