#include "Catch/catch.hpp"

#include <boost/filesystem/operations.hpp>

#include <string>

#include "Repository.hpp"

namespace fs = boost::filesystem;

class TempDirCopy
{
public:
    TempDirCopy(const std::string &from, const std::string &to) : to(to)
    {
        copyDir(from, to);
    }

    TempDirCopy(const TempDirCopy &) = delete;
    TempDirCopy & operator=(const TempDirCopy &) = delete;

    ~TempDirCopy()
    {
        try {
            fs::remove_all(to);
        } catch (...) {
            // Don't throw from the destructor.
        }
    }

private:
    void copyDir(const fs::path &src, const fs::path &dst)
    {
        fs::create_directory(dst);
        for (fs::directory_entry &e : fs::directory_iterator(src)) {
            fs::path path = e.path();
            if (fs::is_directory(path)) {
                copyDir(path, dst/path.filename());
            } else {
                fs::copy_file(path, dst/path.filename());
            }
        }
    }

private:
    const std::string to;
};

TEST_CASE("Repository is discovered by nested path", "[Repository]")
{
    TempDirCopy tempDirCopy("tests/test-repo/_git", "tests/test-repo/.git");

    REQUIRE_NOTHROW(Repository repo("tests/test-repo/subdir"));
}
