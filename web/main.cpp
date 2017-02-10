#include <boost/program_options.hpp>
#include <cxxtools/log.h>
#include <tnt/tntnet.h>

#include <cstdlib>

#include <memory>
#include <type_traits>

#include "BuildHistory.hpp"
#include "DB.hpp"
#include "Repository.hpp"
#include "WebSettings.hpp"
#include "decoration.hpp"
#include "printing.hpp"

namespace po = boost::program_options;

static po::variables_map parseOptions(const std::vector<std::string> &args);

Repository *globalRepo;
BuildHistory *globalBH;

static po::options_description cmdlineOptions = []() {
    po::options_description opts;
    opts.add_options()
        ("help,h", "display help message")
        ("version,v", "display version")
        ("vhost", po::value<std::string>()->required(), "virtual host name")
        ("ip", po::value<std::string>()->default_value("0.0.0.0"),
         "ip address to bind to")
        ("repo", po::value<std::string>()->default_value("."),
         "path to repository")
        ("port", po::value<int>()->default_value(8000),
         "port to listen to (8000 by default)");
    return opts;
}();

template <typename T>
struct vectorArgs
{
    static const bool value = std::is_same<typename T::args_type,
                                           std::vector<std::string>>::value;
};

template <typename T>
typename std::enable_if<vectorArgs<T>::value, void>::type
setArgs(T &mt, const std::map<std::string, std::string> &args)
{
    std::vector<std::string> argsVector;
    for (const auto entry : args) {
        argsVector.push_back(entry.first + '=' + entry.second);
    }
    mt.setArgs(argsVector);
}

template <typename T>
typename std::enable_if<!vectorArgs<T>::value, void>::type
setArgs(T &mt, const std::map<std::string, std::string> &args)
{
    mt.setArgs(args);
}

int
main(int argc, char *argv[]) try
{
    po::variables_map varMap = parseOptions({ argv + 1, argv + argc });

    if (varMap.count("help")) {
        std::cout << cmdlineOptions;
        return EXIT_SUCCESS;
    }

    if (varMap.count("version")) {
        std::cout << "uncov-web v0.1\n";
        return EXIT_SUCCESS;
    }

    decor::disableDecorations();

    auto settings = std::make_shared<WebSettings>();
    PrintingSettings::set(settings);

    Repository repo(varMap["repo"].as<std::string>());
    DB db(repo.getGitPath() + "/uncov.sqlite");
    BuildHistory bh(db);

    std::string vhost = varMap["vhost"].as<std::string>();
    std::string ip = varMap["ip"].as<std::string>();
    int port = varMap["port"].as<int>();

    globalRepo = &repo;
    globalBH = &bh;

    using tnt::Maptarget;

    log_init();
    tnt::Tntnet app;
    app.setAppName("uncover-web");
    app.listen(ip, port);
    app.vMapUrl(vhost, "^/$", Maptarget("builds"));
    app.vMapUrl(vhost, "^/builds/?$", Maptarget("builds"));
    setArgs(app.vMapUrl(vhost, "^/builds/([0-9]+)/?$", Maptarget("build")),
            {{ "buildId", "$1" }});
    setArgs(app.vMapUrl(vhost, "^/builds/([0-9]+)/(.+)/$",
                        Maptarget("build")),
            {{ "buildId", "$1" }, { "dirPath", "$2" }});
    setArgs(app.vMapUrl(vhost, "^/builds/([0-9]+)/(.+)$",
                        Maptarget("file")),
            {{ "buildId", "$1" }, { "filePath", "$2" }});
    app.vMapUrl(vhost, "^/style.css", Maptarget("style"));
    app.vMapUrl(vhost, "^/favicon.ico", Maptarget("favicon"));
    app.run();
} catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
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
    po::positional_options_description positional_options;
    auto parsed_from_cmdline =
        po::command_line_parser(args)
        .options(cmdlineOptions)
        .positional(positional_options)
        .run();

    po::variables_map varMap;
    po::store(parsed_from_cmdline, varMap);

    if (!varMap.count("vhost") && !varMap.count("help") &&
        !varMap.count("version")) {
        throw std::runtime_error("--vhost option is required");
    }

    return varMap;
}
