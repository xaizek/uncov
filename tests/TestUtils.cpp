// Copyright (C) 2016 xaizek <xaizek@posteo.net>
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

#include "TestUtils.hpp"

#include <boost/filesystem/operations.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/range.hpp>

#include <fstream>
#include <ostream>

#include "Settings.hpp"

namespace fs = boost::filesystem;

static void copyDir(const fs::path &src, const fs::path &dst);

TempDirCopy::TempDirCopy(const std::string &from, const std::string &to,
                         bool force)
    : to(to)
{
    if (force) {
        fs::remove_all(to);
    }
    copyDir(from, to);
}

static void
copyDir(const fs::path &src, const fs::path &dst)
{
    using it = fs::directory_iterator;

    fs::create_directory(dst);
    for (fs::directory_entry &e : boost::make_iterator_range(it(src), it())) {
        fs::path path = e.path();
        if (fs::is_directory(path)) {
            copyDir(path, dst/path.filename());
        } else {
            fs::copy_file(path, dst/path.filename());
        }
    }
}

TempDirCopy::~TempDirCopy()
{
    try {
        fs::remove_all(to);
    } catch (...) {
        // Don't throw from the destructor.
    }
}

FileRestorer::FileRestorer(const std::string &from, const std::string &to)
    : from(from), to(to)
{
    fs::copy_file(from, to);
}

FileRestorer::~FileRestorer()
{
    fs::remove(from);
    fs::rename(to, from);
}

Settings &
getSettings()
{
    static TestSettings settings;
    return settings;
}

void
makeGz(const std::string &path, const std::string &contents)
{
    std::ofstream file(path, std::ios_base::out | std::ios_base::binary);

    boost::iostreams::filtering_ostreambuf out;
    out.push(boost::iostreams::gzip_compressor());
    out.push(file);

    std::basic_ostream<char>(&out) << contents;
}
