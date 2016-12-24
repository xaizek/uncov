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

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/optional.hpp>

#include <cassert>

#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <set>
#include <string>
#include <vector>

#include "utils/fs.hpp"
#include "BuildHistory.hpp"
#include "FilePrinter.hpp"
#include "Repository.hpp"
#include "SubCommand.hpp"
#include "TablePrinter.hpp"
#include "arg_parsing.hpp"
#include "coverage.hpp"
#include "integration.hpp"
#include "listings.hpp"

/**
 * @brief Type of path in some build.
 */
enum class PathCategory
{
    File,      ///< Path refers to a file.
    Directory, ///< Path refers to a directory.
    None       ///< Path is not in the build.
};

class InRepoPath
{
    using fsPath = boost::filesystem::path;

public:
    explicit InRepoPath(const Repository *repo) : repo(repo)
    {
    }

public:
    InRepoPath & operator=(std::string path)
    {
        namespace fs = boost::filesystem;
        fsPath absRepoRoot =
            fs::absolute(normalize(repo->getGitPath())).parent_path();

        if (path.substr(0, 1) == "/") {
            path.erase(path.begin());
        } else if (pathIsInSubtree(absRepoRoot, fs::current_path())) {
            path = relative(absRepoRoot, fs::absolute(path)).string();
        }

        this->path = normalize(path).string();
        return *this;
    }

    operator std::string() const
    {
        return path;
    }

    const std::string & str() const
    {
        return path;
    }

    bool empty() const
    {
        return path.empty();
    }

private:
    static fsPath normalize(const fsPath &path)
    {
        fsPath result;
        for (fsPath::iterator it = path.begin(); it != path.end(); ++it) {
            if (*it == "..") {
                if(result.filename() == "..") {
                    result /= *it;
                } else {
                    result = result.parent_path();
                }
            } else if (*it != ".") {
                result /= *it;
            }
        }
        return result;
    }

    static fsPath relative(fsPath base, fsPath path)
    {
        auto baseIt = base.begin();
        auto pathIt = path.begin();

        // Loop through both
        while (baseIt != base.end() && pathIt != path.end() &&
               *pathIt == *baseIt) {
            ++pathIt;
            ++baseIt;
        }

        fsPath finalPath;
        while (baseIt != base.end()) {
            finalPath /= "..";
            ++baseIt;
        }

        while (pathIt != path.end()) {
            finalPath /= *pathIt;
            ++pathIt;
        }

        return finalPath;
    }

private:
    const Repository *repo;
    std::string path;
};

static Build getBuild(BuildHistory *bh, int buildId);
static File & getFile(const Build &build, const std::string &path);
static void printFile(BuildHistory *bh, const Repository *repo,
                      const Build &build, const File &file,
                      FilePrinter &printer);
static void printLineSeparator();
static PathCategory classifyPath(const Build &build, const std::string &path);

class BuildsCmd : public AutoSubCommand<BuildsCmd>
{
    struct All { static constexpr const char *const text = "all"; };

public:
    BuildsCmd() : AutoSubCommand("builds", 0U, 1U)
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
        } else if (!args.empty()) {
            std::cerr << "Invalid arguments for subcommand.\n";
            return error();
        }

        TablePrinter tablePrinter {
            { "Build", "Coverage", "C/R Lines", "Cov Change",
              "C/U/R Line Changes", "Branch", "Commit", "Time" },
            getTerminalSize().first
        };

        std::vector<Build> builds = bh->getBuilds();
        if (limitBuilds && builds.size() > maxBuildCount) {
            builds.erase(builds.cbegin(), builds.cend() - maxBuildCount);
        }

        for (Build &build : builds) {
            tablePrinter.append(describeBuild(bh, build));
        }

        RedirectToPager redirectToPager;
        tablePrinter.print(std::cout);
    }
};

class DiffCmd : public AutoSubCommand<DiffCmd>
{
public:
    DiffCmd() : AutoSubCommand("diff", 0U, 3U)
    {
    }

private:
    virtual void
    execImpl(const std::vector<std::string> &args) override
    {
        bool findPrev = false;
        bool buildsDiff = false;
        int oldBuildId, newBuildId;
        InRepoPath path(repo);
        if (auto parsed = tryParse<FilePath>(args)) {
            findPrev = true;
            newBuildId = LatestBuildMarker;
            std::tie(path) = *parsed;
        } else if (auto parsed = tryParse<BuildId, BuildId, FilePath>(args)) {
            std::tie(oldBuildId, newBuildId, path) = *parsed;
        } else if (auto parsed = tryParse<BuildId, BuildId>(args)) {
            findPrev = true;
            buildsDiff = true;
            std::tie(oldBuildId, newBuildId) = *parsed;
        } else if (args.empty()) {
            findPrev = true;
            buildsDiff = true;
            newBuildId = LatestBuildMarker;
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

        RedirectToPager redirectToPager;

        if (buildsDiff) {
            diffBuilds(oldBuild, newBuild, path);
        } else {
            diffFile(oldBuild, newBuild, path, true);
        }

        // TODO: maybe print some totals/stats here.
    }

    void diffBuilds(const Build &oldBuild, const Build &newBuild,
                    const std::string &dirFilter)
    {
        const std::vector<std::string> &oldPaths = oldBuild.getPaths();
        const std::vector<std::string> &newPaths = newBuild.getPaths();

        std::set<std::string> allFiles(oldPaths.cbegin(), oldPaths.cend());
        allFiles.insert(newPaths.cbegin(), newPaths.cend());

        printInfo(oldBuild, newBuild, std::string(), true, false);

        for (const std::string &path : allFiles) {
            if (pathIsInSubtree(dirFilter, path)) {
                diffFile(oldBuild, newBuild, path, false);
            }
        }
    }

    void diffFile(const Build &oldBuild, const Build &newBuild,
                  const std::string &filePath, bool standalone)
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

        Text oldVersion = oldFile ? repo->readFile(oldBuild.getRef(), filePath)
                                  : std::string();
        Text newVersion = newFile ? repo->readFile(newBuild.getRef(), filePath)
                                  : std::string();

        if (oldVersion.size() != oldCov.size() ||
            newVersion.size() != newCov.size()) {
            std::cerr << "Coverage information for file " << filePath
                      << " is not accurate\n";
            return error();
        }

        printInfo(oldBuild, newBuild, filePath, standalone, true);
        if (!standalone) {
            std::cout << '\n';
        }

        FilePrinter filePrinter;
        filePrinter.printDiff(std::cout, filePath, oldVersion,
                              oldCov, newVersion, newCov);
    }

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
            printBuildHeader(std::cout, bh, newBuild);
        }
        if (fileInfo) {
            printFileHeader(std::cout, bh, newBuild, filePath);
        }
        printLineSeparator();
    }
};

class DirsCmd : public AutoSubCommand<DirsCmd>
{
public:
    DirsCmd() : AutoSubCommand("dirs", 0U, 1U)
    {
    }

private:
    virtual void
    execImpl(const std::vector<std::string> &args) override
    {
        int buildId;
        InRepoPath dirFilter(repo);
        if (auto parsed = tryParse<BuildId>(args)) {
            buildId = std::get<0>(*parsed);
        } else if (auto parsed = tryParse<BuildId, FilePath>(args)) {
            std::tie(buildId, dirFilter) = *parsed;
        } else {
            std::cerr << "Invalid arguments for subcommand.\n";
            return error();
        }

        Build build = getBuild(bh, buildId);

        TablePrinter tablePrinter({ "-Directory", "Coverage", "C/R Lines",
                                    "Cov Change", "C/U/R Line Changes" },
                                  getTerminalSize().first);

        for (std::vector<std::string> &dirRow :
             describeBuildDirs(bh, build, dirFilter)) {
            tablePrinter.append(std::move(dirRow));
        }

        RedirectToPager redirectToPager;
        tablePrinter.print(std::cout);
    }
};

class FilesCmd : public AutoSubCommand<FilesCmd>
{
public:
    FilesCmd() : AutoSubCommand("files", 0U, 1U)
    {
    }

private:
    virtual void
    execImpl(const std::vector<std::string> &args) override
    {
        int buildId;
        InRepoPath dirFilter(repo);
        if (auto parsed = tryParse<BuildId>(args)) {
            buildId = std::get<0>(*parsed);
        } else if (auto parsed = tryParse<BuildId, FilePath>(args)) {
            std::tie(buildId, dirFilter) = *parsed;
        } else {
            std::cerr << "Invalid arguments for subcommand.\n";
            return error();
        }

        Build build = getBuild(bh, buildId);

        TablePrinter tablePrinter({ "-File", "Coverage", "C/R Lines",
                                    "Cov Change", "C/U/R Line Changes" },
                                  getTerminalSize().first);

        for (std::vector<std::string> &fileRow :
             describeBuildFiles(bh, build, dirFilter)) {
            tablePrinter.append(std::move(fileRow));
        }

        RedirectToPager redirectToPager;
        tablePrinter.print(std::cout);
    }
};

class GetCmd : public AutoSubCommand<GetCmd>
{
public:
    GetCmd() : AutoSubCommand("get", 2U)
    {
    }

private:
    virtual void
    execImpl(const std::vector<std::string> &args) override
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

class NewCmd : public AutoSubCommand<NewCmd>
{
public:
    NewCmd() : AutoSubCommand("new")
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

class ShowCmd : public AutoSubCommand<ShowCmd>
{
public:
    ShowCmd() : AutoSubCommand("show", 0U, 2U)
    {
    }

private:
    virtual void
    execImpl(const std::vector<std::string> &args) override
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

        FilePrinter printer;
        RedirectToPager redirectToPager;
        printBuildHeader(std::cout, bh, build);

        if (printWholeBuild) {
            for (const std::string &path : build.getPaths()) {
                printFile(bh, repo, build, *build.getFile(path), printer);
            }
        } else if (fileType == PathCategory::Directory) {
            for (const std::string &filePath : build.getPaths()) {
                if (pathIsInSubtree(path.str(), filePath)) {
                    printFile(bh, repo, build, *build.getFile(filePath),
                              printer);
                }
            }
        } else {
            printFile(bh, repo, build, *build.getFile(path), printer);
        }
    }
};

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

static void
printFile(BuildHistory *bh, const Repository *repo, const Build &build,
          const File &file, FilePrinter &printer)
{
    printLineSeparator();
    printFileHeader(std::cout, bh, build, file);
    printLineSeparator();

    const std::string &path = file.getPath();
    const std::string &ref = build.getRef();
    printer.print(std::cout, path, repo->readFile(ref, path),
                  file.getCoverage());
}

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
