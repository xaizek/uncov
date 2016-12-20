#define CATCH_CONFIG_RUNNER
#include "Catch/catch.hpp"

#include "decoration.hpp"

#include "TestUtils.hpp"

int
main(int argc, const char *argv[])
{
    decor::disableDecorations();
    TempDirCopy tempDirCopy("tests/test-repo/_git", "tests/test-repo/.git");

    return Catch::Session().run(argc, argv);
}
