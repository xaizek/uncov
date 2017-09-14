// Copyright (C) 2016 xaizek <xaizek@posteo.net>
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

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/optional.hpp>

#include <cassert>
#include <cstddef>

#include <algorithm>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <set>
#include <string>
#include <vector>

#include "utils/Text.hpp"
#include "utils/fs.hpp"
#include "utils/md5.hpp"
#include "BuildHistory.hpp"
#include "FileComparator.hpp"
#include "FilePrinter.hpp"
#include "Repository.hpp"
#include "Settings.hpp"
#include "SubCommand.hpp"
#include "TablePrinter.hpp"
#include "arg_parsing.hpp"
#include "coverage.hpp"
#include "integration.hpp"
#include "listings.hpp"

/**
 * @file sub_commands.cpp
 *
 * @brief Implementation of sub-commands.
 */

namespace {

/**
 * @brief Type of path in some build.
 */
enum class PathCategory
{
    File,      //!< Path refers to a file.
    Directory, //!< Path refers to a directory.
    None       //!< Path is not in the build.
};

/**
 * @brief Helper class that performs implicit conversions of paths.
 */
class InRepoPath
{
public:
    /**
     * @brief Constructs with reference to repository.
     *
     * @param repo Repository to adjust paths for.
     */
    explicit InRepoPath(const Repository *repo) : repo(repo)
    {
    }

public:
    /**
     * @brief Assigns value of the path.
     *
     * @param path New value.
     *
     * @returns @c *this.
     */
    InRepoPath & operator=(std::string path)
    {
        namespace fs = boost::filesystem;
        fs::path absRepoRoot =
            fs::absolute(normalizePath(repo->getGitPath())).parent_path();

        if (path.substr(0, 1) == "/") {
            path.erase(path.begin());
        } else if (pathIsInSubtree(absRepoRoot, fs::current_path())) {
            path = makeRelativePath(absRepoRoot, fs::absolute(path)).string();
        }

        this->path = normalizePath(path).string();
        return *this;
    }

    /**
     * @brief Implicit conversion to a string.
     *
     * @returns The path value.
     */
    operator std::string() const
    {
        return path;
    }

    /**
     * @brief Explicit conversion to a string.
     *
     * @returns The path value.
     */
    const std::string & str() const
    {
        return path;
    }

    /**
     * @brief Checks whether the path is empty.
     *
     * @returns @c true if so, @c false otherwise.
     */
    bool empty() const
    {
        return path.empty();
    }

private:
    const Repository *repo; //!< Repository for which paths are adjusted.
    std::string path;       //!< Stored path.
};

}

static Build getBuild(BuildHistory *bh, int buildId);
static File & getFile(const Build &build, const std::string &path);
static void printFile(BuildHistory *bh, const Repository *repo,
                      const Build &build, const File &file,
                      FilePrinter &printer, bool leaveMissedOnly);
static void printLineSeparator();
static PathCategory classifyPath(const Build &build, const std::string &path);

/**
 * @brief Displays information about single build.
 */
class BuildCmd : public AutoSubCommand<BuildCmd>
{
public:
    BuildCmd() : AutoSubCommand({ "build" }, 0U, 1U)
    {
        describe("build", "Displays information about single build");
    }

private:
    virtual void
    execImpl(const std::string &/*alias*/,
             const std::vector<std::string> &args) override
    {
        int buildId;
        if (auto parsed = tryParse<BuildId>(args)) {
            std::tie(buildId) = *parsed;
        } else {
            std::cerr << "Invalid arguments for subcommand.\n";
            return error();
        }

        TablePrinter tablePrinter {
            { "-Name", "-Value" }, getTerminalSize().first, true
        };

        Build build = getBuild(bh, buildId);

        const std::vector<std::string> descr = describeBuild(bh, build,
                                                             !DoExtraAlign{},
                                                             DoSpacing{});
        tablePrinter.append({ "Id:", descr[0] });
        tablePrinter.append({ "Coverage:", descr[1] });
        tablePrinter.append({ "C/R Lines:", descr[2] });
        tablePrinter.append({ "Cov Change:", descr[3] });
        tablePrinter.append({ "C/M/R Line Changes:", descr[4] });
        tablePrinter.append({ "Ref:", descr[5] });
        tablePrinter.append({ "Commit:", descr[6] });
        tablePrinter.append({ "Time:", descr[7] });

        RedirectToPager redirectToPager;
        tablePrinter.print(std::cout);
    }
};

/**
 * @brief Lists builds.
 */
class BuildsCmd : public AutoSubCommand<BuildsCmd>
{
    //! Storage for `"all"` literal.
    struct All
    {
        //! The literal.
        static constexpr const char *const text = "all";
    };

public:
    BuildsCmd() : AutoSubCommand({ "builds" }, 0U, 1U)
    {
        describe("builds", "Lists builds");
    }

private:
    virtual void
    execImpl(const std::string &/*alias*/,
             const std::vector<std::string> &args) override
    {
        // By default limit number of builds to display to 10.
        bool limitBuilds = true;
        unsigned int maxBuildCount = 10;
        if (auto parsed = tryParse<PositiveNumber>(args)) {
            std::tie(maxBuildCount) = *parsed;
        } else if (tryParse<StringLiteral<All>>(args)) {
            limitBuilds = false;
        } else if (!args.empty()) {
            std::cerr << "Invalid arguments for subcommand.\n";
            return error();
        }

        TablePrinter tablePrinter {
            { "Build", "Coverage", "C/R Lines", "Cov Change",
              "C/M/R Line Changes", "Ref" },
            getTerminalSize().first
        };

        std::vector<Build> builds = bh->getBuilds();
        if (limitBuilds && builds.size() > maxBuildCount) {
            builds.erase(builds.cbegin(), builds.cend() - maxBuildCount);
        }

        for (Build &build : builds) {
            const std::vector<std::string> descr = describeBuild(bh, build,
                                                                 DoExtraAlign{},
                                                                 DoSpacing{});
            tablePrinter.append({ descr[0], descr[1], descr[2], descr[3],
                                  descr[4], descr[5] });
        }

        RedirectToPager redirectToPager;
        tablePrinter.print(std::cout);
    }
};

/**
 * @brief Compares builds, directories or files.
 */
class DiffCmd : public AutoSubCommand<DiffCmd>
{
public:
    DiffCmd() : AutoSubCommand({ "diff", "diff-hits" }, 0U, 3U)
    {
        describe("diff", "Compares builds, directories or files");
        describe("diff-hits", "Compares builds, directories or files by hits");
    }

private:
    virtual void
    execImpl(const std::string &alias,
             const std::vector<std::string> &args) override
    {
        bool findPrev = false;
        bool buildsDiff = false;
        int oldBuildId, newBuildId;
        InRepoPath path(repo);
        if (args.empty()) {
            findPrev = true;
            buildsDiff = true;
            newBuildId = LatestBuildMarker;
        } else if (auto parsed = tryParse<BuildId>(args)) {
            buildsDiff = true;
            newBuildId = LatestBuildMarker;
            std::tie(oldBuildId) = *parsed;
        } else if (auto parsed = tryParse<BuildId, BuildId>(args)) {
            buildsDiff = true;
            std::tie(oldBuildId, newBuildId) = *parsed;
        } else if (auto parsed = tryParse<FilePath>(args)) {
            findPrev = true;
            newBuildId = LatestBuildMarker;
            std::tie(path) = *parsed;
        } else if (auto parsed = tryParse<BuildId, BuildId, FilePath>(args)) {
            std::tie(oldBuildId, newBuildId, path) = *parsed;
        } else {
            std::cerr << "Invalid arguments for subcommand.\n";
            return error();
        }

        Build newBuild = getBuild(bh, newBuildId);

        if (findPrev) {
            oldBuildId = bh->getPreviousBuildId(newBuild.getId());
            if (oldBuildId == 0) {
                std::cerr << "Failed to obtain previous build of #"
                          << newBuild.getId() << '\n';
                return error();
            }
        }

        Build oldBuild = getBuild(bh, oldBuildId);

        if (!buildsDiff) {
            PathCategory oldType = classifyPath(oldBuild, path);
            PathCategory newType = classifyPath(newBuild, path);

            if (oldType == PathCategory::None &&
                newType == PathCategory::None) {
                std::cerr << "No " << path.str() << " file in both builds (#"
                          << oldBuild.getId() << " and #" << newBuild.getId()
                          << ")\n";
                return error();
            }

            if (oldType != PathCategory::File &&
                newType != PathCategory::File) {
                buildsDiff = true;
            }
        }

        filePrinter.reset(new FilePrinter(*settings));

        RedirectToPager redirectToPager;

        CompareStrategy strategy = (alias == "diff")
                                 ? CompareStrategy::State
                                 : CompareStrategy::Hits;

        if (buildsDiff) {
            diffBuilds(oldBuild, newBuild, path, strategy);
        } else {
            diffFile(oldBuild, newBuild, path, true, strategy);
        }

        filePrinter.reset();

        // TODO: maybe print some totals/stats here.
    }

    /**
     * @brief Prints difference between two builds.
     *
     * @param oldBuild  Original build.
     * @param newBuild  Changed build.
     * @param dirFilter Prefix to filter paths.
     * @param strategy  Comparison strategy.
     */
    void diffBuilds(const Build &oldBuild, const Build &newBuild,
                    const std::string &dirFilter, CompareStrategy strategy)
    {
        const std::vector<std::string> &oldPaths = oldBuild.getPaths();
        const std::vector<std::string> &newPaths = newBuild.getPaths();

        std::set<std::string> allFiles(oldPaths.cbegin(), oldPaths.cend());
        allFiles.insert(newPaths.cbegin(), newPaths.cend());

        printInfo(oldBuild, newBuild, std::string(), true, false);

        for (const std::string &path : allFiles) {
            if (pathIsInSubtree(dirFilter, path)) {
                diffFile(oldBuild, newBuild, path, false, strategy);

                // Flush output stream so that user can start seeing output
                // faster than output buffer fills up (this is actually
                // noticeable as composing diffs and highlighting files takes
                // time).
                std::cout.flush();
            }
        }
    }

    /**
     * @brief Prints difference of a file between two builds.
     *
     * @param oldBuild     Original build.
     * @param newBuild     Changed build.
     * @param filePath     Path to the file.
     * @param standalone   Whether we're printing just one file.
     * @param strategy  Comparison strategy.
     */
    void diffFile(const Build &oldBuild, const Build &newBuild,
                  const std::string &filePath, bool standalone,
                  CompareStrategy strategy)
    {
        boost::optional<File &> oldFile = oldBuild.getFile(filePath);
        boost::optional<File &> newFile = newBuild.getFile(filePath);

        const std::string &oldHash = oldFile ? oldFile->getHash()
                                             : std::string();
        const std::string &newHash = newFile ? newFile->getHash()
                                             : std::string();
        const std::vector<int> &oldCov = oldFile ? oldFile->getCoverage()
                                                 : std::vector<int>{};
        const std::vector<int> &newCov = newFile ? newFile->getCoverage()
                                                 : std::vector<int>{};
        if (oldHash == newHash && oldCov == newCov) {
            // Do nothing for files that didn't change at all.
            return;
        }

        Text oldVersion(oldFile ? repo->readFile(oldBuild.getRef(), filePath)
                                : std::string());
        Text newVersion(newFile ? repo->readFile(newBuild.getRef(), filePath)
                                : std::string());

        FileComparator comparator(oldVersion.asLines(), oldCov,
                                  newVersion.asLines(), newCov,
                                  strategy, *settings);

        if (!comparator.isValidInput()) {
            std::cerr << "Coverage information for file " << filePath
                      << " is not accurate:\n" << comparator.getInputError();
            return error();
        }

        if (comparator.areEqual()) {
            // Do nothing for files that we don't consider different.
            return;
        }

        if (!standalone) {
            std::cout << '\n';
        }
        printInfo(oldBuild, newBuild, filePath, standalone, true);

        filePrinter->printDiff(std::cout, filePath,
                               oldVersion.asStream(), oldCov,
                               newVersion.asStream(), newCov,
                               comparator);
    }

    /**
     * @brief Prints information about comparison.
     *
     * @param oldBuild  Original build.
     * @param newBuild  Changed build.
     * @param filePath  Path to the file.
     * @param buildInfo Whether to print information about builds.
     * @param fileInfo  Whether to print information about file.
     */
    void printInfo(const Build &oldBuild, const Build &newBuild,
                   const std::string &filePath, bool buildInfo, bool fileInfo)
    {
        assert((buildInfo || fileInfo) && "Expected at least one flag set.");

        printLineSeparator();
        if (buildInfo) {
            printBuildHeader(std::cout, bh, oldBuild);
        }
        if (fileInfo) {
            printFileHeader(std::cout, bh, oldBuild, filePath);
        }
        printLineSeparator();
        if (buildInfo) {
            printBuildHeader(std::cout, bh, newBuild, &oldBuild);
        }
        if (fileInfo) {
            printFileHeader(std::cout, bh, newBuild, filePath, &oldBuild);
        }
        printLineSeparator();
    }

private:
    //! File printer created once shared here to omit passing it around.
    std::unique_ptr<FilePrinter> filePrinter;
};

/**
 * @brief Lists statistics about files or directories.
 */
class FilesCmd : public AutoSubCommand<FilesCmd>
{
public:
    FilesCmd() : AutoSubCommand({ "files", "changed", "dirs" }, 0U, 3U)
    {
        describe("files", "Lists statistics about files");
        describe("changed", "Lists statistics about changed files");
        describe("dirs", "Lists statistics about directories");
    }

private:
    virtual void
    execImpl(const std::string &alias,
             const std::vector<std::string> &args) override
    {
        int buildId;
        InRepoPath dirFilter(repo);
        boost::optional<Build> prevBuild;
        if (auto parsed = tryParse<BuildId>(args)) {
            buildId = std::get<0>(*parsed);
        } else if (auto parsed = tryParse<BuildId, BuildId>(args)) {
            prevBuild = bh->getBuild(std::get<0>(*parsed));
            buildId = std::get<1>(*parsed);
        } else if (auto parsed = tryParse<BuildId, BuildId, FilePath>(args)) {
            int prevBuildId;
            std::tie(prevBuildId, buildId, dirFilter) = *parsed;
            prevBuild = bh->getBuild(prevBuildId);
        } else if (auto parsed = tryParse<BuildId, FilePath>(args)) {
            std::tie(buildId, dirFilter) = *parsed;
        } else {
            std::cerr << "Invalid arguments for subcommand.\n";
            return error();
        }

        Build build = getBuild(bh, buildId);

        if (!dirFilter.empty()) {
            if (alias == "dirs") {
                if (classifyPath(build, dirFilter) != PathCategory::Directory) {
                    std::cerr << "Specified path wasn't found in the build.\n";
                    return error();
                }
            } else {
                if (classifyPath(build, dirFilter) == PathCategory::None) {
                    std::cerr << "Specified path wasn't found in the build.\n";
                    return error();
                }
            }
        }

        const std::string firstCol = (alias == "dirs") ? "-Directory" : "-File";
        TablePrinter tablePrinter({ firstCol, "Coverage", "C/R Lines",
                                    "Cov Change", "C/M/R Line Changes" },
                                  getTerminalSize().first);

        std::vector<std::vector<std::string>> table;
        Build *prev = (prevBuild ? &*prevBuild : nullptr);

        if (alias == "dirs") {
            table = describeBuildDirs(bh, build, dirFilter, prev);
        } else {
            ListChangedOnly changedOnly(alias == "changed");
            table = describeBuildFiles(bh, build, dirFilter, changedOnly,
                                       !ListDirectOnly{}, prev);
        }

        for (std::vector<std::string> &row : table) {
            tablePrinter.append(std::move(row));
        }

        RedirectToPager redirectToPager;
        tablePrinter.print(std::cout);
    }
};

/**
 * @brief Dumps coverage information of a file.
 */
class GetCmd : public AutoSubCommand<GetCmd>
{
public:
    GetCmd() : AutoSubCommand({ "get" }, 2U)
    {
        describe("get", "Dumps coverage information of a file");
    }

private:
    virtual void
    execImpl(const std::string &/*alias*/,
             const std::vector<std::string> &args) override
    {
        int buildId;
        InRepoPath filePath(repo);
        if (auto parsed = tryParse<BuildId, FilePath>(args)) {
            std::tie(buildId, filePath) = *parsed;
        } else {
            std::cerr << "Invalid arguments for subcommand.\n";
            return error();
        }

        Build build = getBuild(bh, buildId);
        File &file = getFile(build, filePath);

        std::cout << build.getRef() << '\n';
        for (int hits : file.getCoverage()) {
            std::cout << hits << '\n';
        }
    }
};

/**
 * @brief Imports new build from stdin.
 */
class NewCmd : public AutoSubCommand<NewCmd>
{
public:
    NewCmd() : AutoSubCommand({ "new" })
    {
        describe("new", "Imports new build from stdin");
    }

private:
    virtual void
    execImpl(const std::string &/*alias*/,
             const std::vector<std::string> &/*args*/) override
    {
        std::string ref, refName;
        std::cin >> ref >> refName;

        const std::unordered_map<std::string, std::string> files =
            repo->listFiles(ref);

        BuildData bd(std::move(ref), refName);

        for (std::string path, hash; std::cin >> path >> hash; ) {
            // Normalize path in place (via temporary object).
            path = (InRepoPath(repo) = path);

            int nLines;
            if (!(std::cin >> nLines) || nLines < 0) {
                std::cerr << "Invalid input format\n";
                error();
                break;
            }

            std::vector<int> coverage;
            coverage.reserve(nLines);

            while (nLines-- > 0) {
                int i;
                if (!(std::cin >> i)) {
                    std::cerr << "Invalid input format\n";
                    error();
                    break;
                }
                coverage.push_back(i);
            }

            const auto file = files.find(path);
            if (file == files.cend()) {
                std::cerr << "Skipping file missing in " << refName << ": "
                          << path << '\n';
            } else if (!boost::iequals(file->second, hash)) {
                std::cerr << path << " file at " << refName
                          << " doesn't match reported MD5 hash\n";
                error();
            } else {
                bd.addFile(File(std::move(path), std::move(hash),
                                std::move(coverage)));
            }
        }

        if (!isFailed()) {
            Build build = bh->addBuild(bd);
            printBuildHeader(std::cout, bh, build);
        }
    }
};

/**
 * @brief Imports new build in JSON format from stdin.
 */
class NewJsonCmd : public AutoSubCommand<NewJsonCmd>
{
public:
    NewJsonCmd() : AutoSubCommand({ "new-json" })
    {
        describe("new-json", "Imports new build in JSON format from stdin");
    }

private:
    virtual void
    execImpl(const std::string &/*alias*/,
             const std::vector<std::string> &/*args*/) override
    {
        namespace pt = boost::property_tree;

        if (std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '{')) {
            std::cin.putback('{');
        }

        pt::ptree props;
        pt::read_json(std::cin, props);

        std::string ref = props.get<std::string>("git.head.id");
        std::string refName = props.get<std::string>("git.branch");

        const std::unordered_map<std::string, std::string> files =
            repo->listFiles(ref);

        BuildData bd(std::move(ref), refName);

        for (auto &s : props.get_child("source_files")) {
            // Normalize path in place (via temporary object).
            const std::string path =
                (InRepoPath(repo) = s.second.get<std::string>("name"));

            std::string hash;
            bool computedHash = false;
            if (auto d = s.second.get_optional<std::string>("source_digest")) {
                hash = *d;
            } else {
                auto contents = s.second.get<std::string>("source");
                hash = md5(contents);
                computedHash = true;
            }

            const auto file = files.find(path);
            if (file == files.cend()) {
                std::cerr << "Skipping file missing in " << refName << ": "
                          << path << '\n';
                continue;
            } else if (!boost::iequals(file->second, hash)) {
                if (computedHash) {
                    auto contents = s.second.get<std::string>("source");
                    hash = md5(contents + '\n');
                }

                if (!boost::iequals(file->second, hash)) {
                    std::cerr << path << " file at " << refName
                              << " doesn't match reported contents\n";
                    error();
                    continue;
                }
            }

            auto cov = s.second.get_child("coverage");

            std::vector<int> coverage;
            coverage.reserve(cov.size());

            for (auto &hits : cov) {
                if (auto val = hits.second.get_value_optional<int>()) {
                    coverage.push_back(*val);
                } else {
                    coverage.push_back(-1);
                }
            }

            bd.addFile(File(std::move(path), std::move(hash),
                            std::move(coverage)));
        }

        if (!isFailed()) {
            Build build = bh->addBuild(bd);
            printBuildHeader(std::cout, bh, build);
        }
    }
};

/**
 * @brief Displays a build, directory or file.
 */
class ShowCmd : public AutoSubCommand<ShowCmd>
{
public:
    ShowCmd() : AutoSubCommand({ "missed", "show" }, 0U, 2U)
    {
        describe("missed", "Displays missed in a build, directory or file");
        describe("show", "Displays a build, directory or file");
    }

private:
    virtual void
    execImpl(const std::string &alias,
             const std::vector<std::string> &args) override
    {
        int buildId;
        InRepoPath path(repo);
        bool printWholeBuild = false;
        if (auto parsed = tryParse<BuildId>(args)) {
            buildId = std::get<0>(*parsed);
            printWholeBuild = true;
        } else if (auto parsed = tryParse<FilePath>(args)) {
            buildId = 0;
            path = std::get<0>(*parsed);
        } else if (auto parsed = tryParse<BuildId, FilePath>(args)) {
            std::tie(buildId, path) = *parsed;
        } else {
            std::cerr << "Invalid arguments for subcommand.\n";
            return error();
        }

        Build build = getBuild(bh, buildId);

        PathCategory fileType = path.empty() ? PathCategory::Directory
                                             : classifyPath(build, path);
        if (fileType == PathCategory::None) {
            std::cerr << "No such file " << path.str() << " in build #"
                      << buildId << "\n";
            return error();
        }

        FilePrinter printer(*settings);
        RedirectToPager redirectToPager;
        printBuildHeader(std::cout, bh, build);

        const bool leaveMissedOnly = (alias == "missed");

        if (printWholeBuild) {
            for (const std::string &path : build.getPaths()) {
                printFile(bh, repo, build, *build.getFile(path), printer,
                          leaveMissedOnly);
            }
        } else if (fileType == PathCategory::Directory) {
            for (const std::string &filePath : build.getPaths()) {
                if (pathIsInSubtree(path.str(), filePath)) {
                    printFile(bh, repo, build, *build.getFile(filePath),
                              printer, leaveMissedOnly);
                }
            }
        } else {
            printFile(bh, repo, build, *build.getFile(path), printer,
                      leaveMissedOnly);
        }
    }
};

/**
 * @brief Retrieves build by its id.
 *
 * @param bh      Build history.
 * @param buildId Build id.
 *
 * @returns The build.
 *
 * @throws std::runtime_error If there are no builds or build id is wrong.
 */
static Build
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

/**
 * @brief Retrieves file from a build.
 *
 * @param build Build to look up file in.
 * @param path  Path to look up.
 *
 * @returns The file.
 *
 * @throws std::runtime_error On wrong path.
 */
static File &
getFile(const Build &build, const std::string &path)
{
    boost::optional<File &> file = build.getFile(path);
    if (!file) {
        throw std::runtime_error("Can't find file: " + path + " in build #" +
                                 std::to_string(build.getId()) + " of " +
                                 build.getRefName() + " at " +
                                 build.getRef());
    }
    return *file;
}

/**
 * @brief Prints file onto the screen.
 *
 * @param bh Build history (for querying previous build).
 * @param repo Repository.
 * @param build Build.
 * @param file File to print.
 * @param printer File printer.
 * @param leaveMissedOnly Fold lines which are covered or not relevant.
 */
static void
printFile(BuildHistory *bh, const Repository *repo, const Build &build,
          const File &file, FilePrinter &printer, bool leaveMissedOnly)
{
    const std::vector<int> &coverage = file.getCoverage();

    if (leaveMissedOnly && std::none_of(coverage.cbegin(), coverage.cend(),
                                        [](int x) { return x == 0; })) {
        // Do nothing for files that don't have any missed lines.
        return;
    }

    printLineSeparator();
    printFileHeader(std::cout, bh, build, file);
    printLineSeparator();

    const std::string &path = file.getPath();
    const std::string &ref = build.getRef();
    printer.print(std::cout, path, repo->readFile(ref, path), coverage,
                  leaveMissedOnly);
}

/**
 * @brief Prints horizontal separator.
 */
static void
printLineSeparator()
{
    std::cout << std::setfill('-') << std::setw(80) << '\n'
              << std::setfill(' ');
}

/**
 * @brief Categorizes path within repository as file, directory or absent.
 *
 * @param build Build to look up file in.
 * @param path Path of the file.
 *
 * @returns The category.
 */
static PathCategory
classifyPath(const Build &build, const std::string &path)
{
    for (const std::string &filePath : build.getPaths()) {
        if (filePath == path) {
            return PathCategory::File;
        }

        boost::filesystem::path dirPath = filePath;
        dirPath.remove_filename();
        if (dirPath == path) {
            return PathCategory::Directory;
        }
    }
    return PathCategory::None;
}
