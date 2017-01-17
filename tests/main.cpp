#define CATCH_CONFIG_RUNNER
#include "Catch/catch.hpp"

#include "decoration.hpp"

#include "TestUtils.hpp"

int
main(int argc, const char *argv[])
{
    decor::disableDecorations();

    // Remove destination directory if it exists to account for crashes.
    TempDirCopy tempDirCopy("tests/test-repo/_git", "tests/test-repo/.git",
                            true);

    return Catch::Session().run(argc, argv);
}
