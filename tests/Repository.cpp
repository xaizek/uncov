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
