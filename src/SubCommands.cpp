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

#include <boost/filesystem/path.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream_buffer.hpp>
#include <boost/optional.hpp>
#include <boost/scope_exit.hpp>

#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include "BuildHistory.hpp"
#include "FilePrinter.hpp"
#include "Repository.hpp"
#include "SubCommand.hpp"
#include "TablePrinter.hpp"
#include "decoration.hpp"

static void printBuild(const Build &build);
static void printFile(const Repository *repo, const Build &build,
                      const File &file, FilePrinter &printer);

class CovInfo
{
public:
    CovInfo() = default;
    template <typename T>
    explicit CovInfo(const T &coverable)
        : coveredCount(coverable.getCoveredCount()),
          uncoveredCount(coverable.getUncoveredCount())
    {
    }

public:
    void addCovered(int num)
    {
        coveredCount += num;
    }

    void addUncovered(int num)
    {
        uncoveredCount += num;
    }

    int getCoveredCount() const
    {
        return coveredCount;
    }

    int getUncoveredCount() const
    {
        return uncoveredCount;
    }

    std::string formatCoverageRate() const
    {
        if (getRelevantLines() == 0) {
            return "100";
        }

        auto rate = 100*coveredCount/static_cast<float>(getRelevantLines());

        std::ostringstream oss;
        oss << std::fixed << std::right
            << std::setprecision(2) << rate;
        return oss.str();
    }

    std::string formatLines(const std::string &separator) const
    {
        return std::to_string(coveredCount) + separator
             + std::to_string(getRelevantLines());
    }

private:
    int getRelevantLines() const
    {
        return coveredCount + uncoveredCount;
    }

private:
    int coveredCount = 0;
    int uncoveredCount = 0;
};

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

class BuildsCmd : public AutoSubCommand<BuildsCmd>
{
public:
    BuildsCmd() : parent("builds")
    {
    }

private:
    virtual void
    execImpl(const std::vector<std::string> &/*args*/) override
    {
        // TODO: calculate and display change of coverage (both in percents and
        //       in covered/relevant lines).
        // TODO: by default limit number of builds to display to N (e.g., 10).

        // TODO: colorize percents?
        TablePrinter tablePrinter {
            { "Build", "Coverage", "Lines", "Branch", "Commit" },
            getTerminalWidth()
        };

        std::string sharp("#"), percent(" %"), sep(" / ");
        for (const Build &build : bh->getBuilds()) {
            CovInfo covInfo(build);

            tablePrinter.append({
                sharp + std::to_string(build.getId()),
                covInfo.formatCoverageRate() + percent,
                covInfo.formatLines(sep),
                build.getRefName(),
                build.getRef()
            });
        }

        tablePrinter.print(std::cout);
    }
};

class DirsCmd : public AutoSubCommand<DirsCmd>
{
public:
    DirsCmd() : parent("dirs", 1U)
    {
    }

private:
    virtual void
    execImpl(const std::vector<std::string> &args) override
    {
        namespace fs = boost::filesystem;

        const int buildId = std::stoi(args[0]);
        if (boost::optional<Build> build = bh->getBuild(buildId)) {
            std::map<std::string, CovInfo> dirs;
            for (const std::string &filePath: build->getPaths()) {
                fs::path dirPath = filePath;
                dirPath.remove_filename();

                File &file = *build->getFile(filePath);
                CovInfo &info = dirs[dirPath.string()];

                info.addCovered(file.getCoveredCount());
                info.addUncovered(file.getUncoveredCount());
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
        } else {
            std::cerr << "Can't find build #" << buildId << '\n';
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

        std::unordered_set<std::string> files = repo->listFiles(ref);

        BuildData bd(std::move(ref), refName);

        for (std::string path; std::cin >> path; ) {
            int nLines;
            std::cin >> nLines;

            std::vector<int> coverage;
            coverage.reserve(nLines);

            while (nLines-- > 0) {
                int i;
                std::cin >> i;
                coverage.push_back(i);
            }

            if (files.find(path) != files.end()) {
                bd.addFile(File(std::move(path), std::move(coverage)));
            } else {
                std::cerr << "Skipping file missing in " << refName << ": "
                          << path << '\n';
            }
        }

        Build build = bh->addBuild(bd);
        printBuild(build);
        // TODO: display change of coverage since previous build.
    }
};

class ShowCmd : public AutoSubCommand<ShowCmd>
{
public:
    ShowCmd() : parent("show", 1U, 2U)
    {
    }

private:
    virtual void
    execImpl(const std::vector<std::string> &args) override
    {
        // TODO: maybe allow passing in path to directory, which would cause
        //       printing all files in that sub-tree (or only directly in it?).

        const int buildId = std::stoi(args[0]);
        if (boost::optional<Build> build = bh->getBuild(buildId)) {
            FilePrinter printer;

            if (args.size() == 1U) {
                RedirectToPager redirectToPager;
                printBuild(*build);
                for (const std::string &path : build->getPaths()) {
                    printFile(repo, *build, *build->getFile(path), printer);
                }
            } else if (boost::optional<File &> file = build->getFile(args[1])) {
                RedirectToPager redirectToPager;
                printBuild(*build);
                printFile(repo, *build, *file, printer);
            } else {
                std::cerr << "Can't find file: " << args[1] << '\n';
            }
        } else {
            std::cerr << "Can't find build #" << buildId << '\n';
        }
    }
};

static void
printBuild(const Build &build)
{
    CovInfo covInfo(build);
    // TODO: colorize percents?
    std::cout << (decor::bold << "Build:") << " #" << build.getId() << ", "
              << covInfo.formatCoverageRate() << "% "
              << '(' << covInfo.formatLines("/") << "), "
              << build.getRefName() << " (" << build.getRef() << ")\n";
}

static void
printFile(const Repository *repo, const Build &build, const File &file,
          FilePrinter &printer)
{
    CovInfo covInfo(file);
    const std::string &path = file.getPath();
    const std::string &ref = build.getRef();
    // TODO: colorize percents?
    std::cout << (decor::bold << "File: ") << path << ", "
              << covInfo.formatCoverageRate() << "% "
              << '(' << covInfo.formatLines("/") << ")\n\n";
    printer.print(path, repo->readFile(ref, path), file.getCoverage());
}
