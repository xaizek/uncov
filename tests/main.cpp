#define CATCH_CONFIG_RUNNER
#include "Catch/catch.hpp"

#include <memory>

#include "decoration.hpp"
#include "printing.hpp"

#include "TestUtils.hpp"

int
main(int argc, const char *argv[])
{
    PrintingSettings::set(std::make_shared<TestSettings>());

    decor::disableDecorations();

    // Remove destination directory if it exists to account for crashes.
    TempDirCopy tempDirCopy("tests/test-repo/_git", "tests/test-repo/.git",
                            true);

    return Catch::Session().run(argc, argv);
}
