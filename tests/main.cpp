#define CATCH_CONFIG_RUNNER
#include "Catch/catch.hpp"

#include <boost/algorithm/string/predicate.hpp>

#include <cstdlib>
#include <cstring>

#include <memory>

#include "decoration.hpp"
#include "printing.hpp"

#include "TestUtils.hpp"

int
main(int argc, const char *argv[])
{
    // Drop all `GIT_*` environment variables as they might interfere with
    // running some tests.
    extern char **environ;
    for (char **e = environ; *e != NULL; ++e) {
        if (boost::starts_with(*e, "GIT_")) {
            unsetenv(std::string(*e, std::strchr(*e, '=')).c_str());
        }
    }

    // Fix timezone used by the tests.
    setenv("TZ", "UTC", true);

    PrintingSettings::set(std::make_shared<TestSettings>());

    decor::disableDecorations();

    // Remove destination directory if it exists to account for crashes.
    TempDirCopy tempDirCopy("tests/test-repo/_git", "tests/test-repo/.git",
                            true);

    return Catch::Session().run(argc, argv);
}
