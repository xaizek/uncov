// Copyright (C) 2018 xaizek <xaizek@posteo.net>
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

#include "GcovImporter.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/version.hpp>
#if BOOST_VERSION >= 107200
#  include <boost/filesystem/directory.hpp>
#endif

#include <cassert>

#include <fstream>
#include <istream>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

#include "utils/fs.hpp"
#include "utils/md5.hpp"
#include "utils/strings.hpp"
#include "BuildHistory.hpp"
#include "integration.hpp"

namespace fs = boost::filesystem;
namespace pt = boost::property_tree;

// See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=89961 for information about
// what's wrong with some versions of `gcov` and why binning is needed.

//! `gcov` option to generate coverage in JSON format.
static const char GcovJsonFormat[] = "--json-format";
//! `gcov` option to generate coverage in plain text format.
static const char GcovIntermediateFormat[] = "--intermediate-format";
//! `gcov` option to dump coverage onto standard output.
static const char GcovStdOut[] = "--stdout";

//! First version of `gcov` which has broken `--preserve-paths` option.
static int FirstBrokenGcovVersion = 8;

namespace {
    /**
     * @brief A set of files that should be passed to `gcov` at the same time.
     */
    class Bin {
    public:
        /**
         * @brief Constructs an empty set.
         *
         * @param deduplicateNames Whether to avoid adding name-duplicates.
         */
        Bin(bool deduplicateNames = true) : deduplicateNames(deduplicateNames)
        { }

    public:
        /**
         * @brief Tries to add a file to this bin.
         *
         * @param path Absolute path to the file.
         *
         * @returns `true` if the path was added.
         */
        bool add(const fs::path &path)
        {
            assert(path.is_absolute() && "Paths should be absolute.");

            if (deduplicateNames &&
                !names.emplace(path.filename().string()).second) {
                return false;
            }

            paths.emplace_back(path.string());
            return true;
        }

        /**
         * @brief Retrieves list of paths of this bin.
         *
         * @returns The list.
         */
        const std::vector<std::string> & getPaths() const
        { return paths; }

    private:
        //! Whether no two files should have the same name.
        bool deduplicateNames;
        //! Names of files in this bin if `deduplicateNames` is set.
        std::unordered_set<std::string> names;
        //! Files of this bin.
        std::vector<std::string> paths;
    };

    //
    // Work around Boost renaming no_push() to disable_recursion_pending() and
    // eventually dropping the former by providing a wrapper that prefers the
    // latter.
    //

    template <typename T>
    constexpr decltype(std::declval<T>().disable_recursion_pending(), bool())
    useNoPush(int)
    { return false; }

    template <typename T>
    constexpr bool useNoPush(long)
    { return true; }

    template <typename DT = fs::recursive_directory_iterator>
    typename std::enable_if<useNoPush<DT>(0)>::type stop_recursion(DT &it)
    {
        it.no_push();
    }

    template <typename DT = fs::recursive_directory_iterator>
    typename std::enable_if<!useNoPush<DT>(0)>::type stop_recursion(DT &it)
    {
        it.disable_recursion_pending();
    }
}

GcovInfo::GcovInfo()
    : employBinning(true),
      jsonFormat(false), intermediateFormat(false), stdOut(false)
{
    const std::regex optionRegex("--[-a-z]+");
    const std::regex versionRegex("gcov \\(GCC\\) (.*)");

    std::smatch match;

    const std::string help = readProc({ "gcov", "--help" });
    auto from = help.cbegin();
    auto to = help.cend();
    while (std::regex_search(from, to, match, optionRegex)) {
        const std::string str = match.str();
        if (str == GcovJsonFormat) {
            jsonFormat = true;
        } else if (str == GcovIntermediateFormat) {
            intermediateFormat = true;
        } else if (str == GcovStdOut) {
            stdOut = true;
        }

        from += match.position() + match.length();
    }

    const std::string version = readProc({ "gcov", "--version" });
    if (std::regex_search(version, match, versionRegex)) {
        const int majorVersion = std::stoi(match[1]);
        employBinning = (majorVersion >= FirstBrokenGcovVersion);
    }
}

GcovInfo::GcovInfo(bool employBinning,
                   bool jsonFormat,
                   bool intermediateFormat,
                   bool stdOut)
    : employBinning(employBinning),
      jsonFormat(jsonFormat),
      intermediateFormat(intermediateFormat),
      stdOut(stdOut)
{ }

std::function<GcovImporter::runner_f>
GcovImporter::setRunner(std::function<runner_f> runner)
{
    std::function<runner_f> previous = std::move(getRunner());
    getRunner() = std::move(runner);
    return previous;
}

GcovImporter::GcovImporter(const std::string &root,
                           const std::string &covoutRoot,
                           const std::vector<std::string> &exclude,
                           const std::string &prefix,
                           GcovInfo gcovInfo)
    : gcovInfo(gcovInfo), rootDir(normalizePath(fs::absolute(root))),
      prefix(prefix)
{
    for (const std::string &p : exclude) {
        skipPaths.insert(normalizePath(fs::absolute(p, root)));
    }

    std::unordered_set<std::string> extensions = {
        ".h", ".hh", ".hpp", ".hxx",
        ".c", ".cc", ".cpp", ".cxx",
        ".m", ".mm",
    };
    std::unordered_set<std::string> skipDirs = {
        ".git", ".hg", ".svn", // Various version control systems.
        ".deps"                // Dependency tracking of automake.
    };

    if (!gcovInfo.hasJsonFormat() && !gcovInfo.hasIntermediateFormat()) {
        throw std::runtime_error("Failed to detect machine format of gcov");
    }

    std::vector<fs::path> gcnoFiles;
    for (fs::recursive_directory_iterator it(fs::absolute(covoutRoot)), end;
         it != end; ++it) {
        fs::path path = it->path();
        if (fs::is_directory(path)) {
            if (skipDirs.count(path.filename().string())) {
                stop_recursion(it);
                continue;
            }
        } else if (fs::is_regular_file(path)) {
            // We are looking for *.gcno and not *.gcda, because they are
            // generated even for files that weren't executed (e.g.,
            // `main.cpp`).
            if (path.extension() == ".gcno") {
                gcnoFiles.push_back(path);
            }
        }
    }

    importFiles(std::move(gcnoFiles));

    for (fs::recursive_directory_iterator it(rootDir), end; it != end; ++it) {
        fs::path path = it->path();
        if (fs::is_directory(path)) {
            if (skipDirs.count(path.filename().string())) {
                stop_recursion(it);
            } else if (skipPaths.count(normalizePath(fs::absolute(path)))) {
                stop_recursion(it);
            }
        } else if (extensions.count(path.extension().string())) {
            std::string filePath = makeRelativePath(rootDir, path).string();
            if (mapping.find(filePath) == mapping.end()) {
                std::string contents = readFile(path.string());
                std::string hash = md5(contents);

                int nLines = std::count(contents.cbegin(), contents.cend(),
                                        '\n');

                files.emplace_back(filePath, std::move(hash),
                                   std::vector<int>(nLines, -1));
            }
        }
    }

    for (auto &e : mapping) {
        std::string contents = readFile(root + '/' + e.first);
        std::string hash = md5(contents);

        std::vector<std::string> lines = split(contents, '\n');
        e.second.resize(lines.size(), -1);

        unsigned int i = 0U;
        for (std::string &line : lines) {
            boost::trim_if(line, boost::is_any_of("\r\n \t"));
            if (line == "}" || line == "};") {
                e.second[i] = -1;
            }
            ++i;
        }

        files.emplace_back(e.first, std::move(hash), std::move(e.second));
    }

    mapping.clear();
}

std::vector<File> &&
GcovImporter::getFiles() &&
{
    return std::move(files);
}

void
GcovImporter::importFiles(std::vector<fs::path> gcnoFiles)
{
    if (gcovInfo.hasJsonFormat() && gcovInfo.canPrintToStdOut()) {
        importAsOutput(std::move(gcnoFiles));
    } else {
        importAsFiles(std::move(gcnoFiles));
    }
}

void
GcovImporter::importAsOutput(std::vector<fs::path> gcnoFiles)
{
    std::vector<std::string> cmd = {
        "gcov", GcovJsonFormat, GcovStdOut, "--"
    };

    cmd.reserve(cmd.size() + gcnoFiles.size());
    for (const fs::path &gcnoFile : gcnoFiles) {
        cmd.push_back(gcnoFile.string());
    }

    const std::string output = getRunner()(std::move(cmd), "-");

    for (std::string &json : split(output, '\n')) {
        if (!json.empty()) {
            std::istringstream iss(std::move(json));
            parseGcovJson(iss);
        }
    }
}

void
GcovImporter::importAsFiles(std::vector<fs::path> gcnoFiles)
{
    std::vector<Bin> bins;

    if (gcovInfo.needsBinning()) {
        // We want to execute the runner for tests even if there are no input
        // files.
        bins.emplace_back();

        for (const fs::path &gcnoFile : gcnoFiles) {
            bool added = false;

            for (Bin &bin : bins) {
                if (bin.add(gcnoFile)) {
                    added = true;
                    break;
                }
            }

            if (!added) {
                bins.emplace_back();
                bins.back().add(gcnoFile);
            }
        }
    } else {
        bins.emplace_back(/*deduplicateNames=*/false);
        Bin &bin = bins.back();
        for (const fs::path &gcnoFile : gcnoFiles) {
            bin.add(gcnoFile);
        }
    }

    std::string gcovOption;
    std::string gcovFileExt;
    if (gcovInfo.hasJsonFormat()) {
        gcovOption = GcovJsonFormat;
        gcovFileExt = ".gcov.json.gz";
    } else {
        gcovOption = GcovIntermediateFormat;
        gcovFileExt = ".gcov";
    }

    for (const Bin &bin : bins) {
        const std::vector<std::string> &paths = bin.getPaths();

        std::vector<std::string> cmd = {
            "gcov", "--preserve-paths", gcovOption, "--"
        };
        cmd.insert(cmd.cend(), paths.cbegin(), paths.cend());

        TempDir tempDir("gcovi");
        std::string tempDirPath = tempDir;
        (void)getRunner()(std::move(cmd), tempDirPath);

        for (fs::recursive_directory_iterator it(tempDirPath), end;
             it != end; ++it) {
            fs::path path = it->path();
            if (fs::is_regular_file(path) &&
                boost::ends_with(path.filename().string(), gcovFileExt)) {
                if (gcovInfo.hasJsonFormat()) {
                    parseGcovJsonGz(path.string());
                } else {
                    parseGcov(path.string());
                }
            }
        }
    }
}

void
GcovImporter::parseGcovJsonGz(const std::string &path)
{
    namespace io = boost::iostreams;

    std::ifstream file(path, std::ios_base::in | std::ios_base::binary);

    io::filtering_istreambuf in;
    in.push(io::gzip_decompressor());
    in.push(file);

    std::istream is(&in);
    parseGcovJson(is);
}

void
GcovImporter::parseGcovJson(std::istream &stream)
{
    pt::ptree props;
    pt::read_json(stream, props);

    const std::string cwd = props.get<std::string>("current_working_directory");

    for (auto &file : props.get_child("files")) {
        fs::path path = file.second.get<std::string>("file");
        if (!path.is_absolute()) {
            path = cwd / path;
        }

        const std::string sourcePath = resolveSourcePath(path);
        if (sourcePath.empty()) {
            continue;
        }

        std::vector<int> &coverage = mapping[sourcePath];
        for (auto &line : file.second.get_child("lines")) {
            updateCoverage(coverage,
                           line.second.get<unsigned int>("line_number"),
                           line.second.get<int>("count"));
        }
    }
}

void
GcovImporter::parseGcov(const std::string &path)
{
    std::vector<int> *coverage = nullptr;
    std::ifstream in(path);
    for (std::string line; in >> line; ) {
        std::string type, value;
        std::tie(type, value) = splitAt(line, ':');
        if (type == "file") {
            const std::string sourcePath = resolveSourcePath(value);
            coverage = (sourcePath.empty() ? nullptr : &mapping[sourcePath]);
        } else if (coverage != nullptr && type == "lcount") {
            std::vector<std::string> fields = split(value, ',');
            if (fields.size() < 2U) {
                throw std::runtime_error("Not enough fields in lcount: " +
                                         value);
            }

            unsigned int lineNo = std::stoi(fields[0]);
            int count = std::stoi(fields[1]);
            updateCoverage(*coverage, lineNo, count);
        }
    }
}

std::string
GcovImporter::resolveSourcePath(fs::path unresolved)
{
    if (!unresolved.is_absolute()) {
        unresolved = prefix / unresolved;
    }

    fs::path sourcePath = normalizePath(fs::absolute(unresolved,
                                                     rootDir));
    if (!pathIsInSubtree(rootDir, sourcePath) || isExcluded(sourcePath)) {
        return std::string();
    }

    return makeRelativePath(rootDir, sourcePath).string();
}

void
GcovImporter::updateCoverage(std::vector<int> &coverage, unsigned int lineNo,
                             int count)
{
    if (coverage.size() < lineNo) {
        coverage.resize(lineNo, -1);
    }

    int &entry = coverage[lineNo - 1U];
    entry = (entry == -1 ? count : entry + count);
}

bool
GcovImporter::isExcluded(fs::path path) const
{
    for (const fs::path &skipPath : skipPaths) {
        if (pathIsInSubtree(skipPath, path)) {
            return true;
        }
    }
    return false;
}

std::function<GcovImporter::runner_f> &
GcovImporter::getRunner()
{
    static std::function<runner_f> runner;
    return runner;
}
