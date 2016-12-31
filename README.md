_uncov_
_2016_

**This file updated**: 31 December, 2016

### Brief Description ###

**uncov** is a tool that collect and processes coverage reports.

By storing history of coverage reports it allows one to view how code coverage
changes over time, compare changes that happened and view current state of the
coverage.

The tool is deeply integrated with git repository of processed projects and thus
avoids generation of static reports.  Data is bound to repository information,
but can be freely shared by several copies of the same repository (builds
referring to unavailable git objects just won't be accessible).

Provided command-line interface should be familiar to most of git users and
helps to avoid switching to a browser to verify code coverage.

### Structure ###

Storage management tool itself is language independent and is relying on
complementary tools to fetch and transform coverage data from language specific
coverage tools.

Importer of coverage for C and C++ languages that collects data from gcov is
provided (code is based on [cpp-coveralls][cpp-coveralls]).  Support for other
languages can be added by converting other similar tools.

### Features ###

* Code highlighting.
* Comparison of coverage.

### Status ###

Most of the things are expected to remain as is, however they aren't finalized
and changes for the sake of improvement are possible.  Databases will be
migrated if schema changes, so previously collected data won't be lost.

#### What's missing ####

 * Configuration.  Currently values that could be configurable are hard-coded.

### Supported Environment ###

Expected to work in \*nix like environments.

### Prerequisites ###

* [GNU Make][make];
* C++14 compatible compiler (C++14 is used for several library routines and
  thus requirement might be lowered to C++11 in future; GCC 4.9 works fine);
* [Boost][boost], tested with 1.55 and 1.59, but older versions might work;
* [libgit2][libgit2];
* [libsqlite3][sqlite];
* libsource-highlight from [GNU Source-highlight][srchilite];
* [zlib][zlib];
* (optional) [pandoc][pandoc] for regenerating man page;
* (optional) [python][python] for collecting coverage for C and C++ (would be
  nice to get rid of this weird dependency, probably by rewriting the tool).

### Usage ###

`uncov-gcov` can be used to generate coverage, but it seems to not play well
with out-of-tree builds (some coverage is missing), so the recommended way of
recording coverage information is shown in example below:

    # reset coverage counters from previous runs
    find . -name '*.gcda' -delete
    # < run tests here >
    # generage coverage for every object file found (change "." to build root)
    find . -name '*.o' -exec gcov -p {} +
    # generage and combine coverage reports (--capture-worktree automatically
    # makes stray commit if it's dirty)
    uncov-gcov --root . --build-root . --no-gcov --capture-worktree \
               --exclude tests | uncov new
    # remove coverage reports
    find . -name '*.gcov' -delete

### Credits ###

* [LCOV][lcov] project is the source of useful ideas and primary source of
  inspiration.
* [COVERALLS][coveralls] service is second project which significantly shaped
  this tool.

### License ###

GNU Affero General Public License, version 3 or later.


[lcov]: http://ltp.sourceforge.net/coverage/lcov.php
[coveralls]: https://coveralls.io/
[cpp-coveralls]: https://github.com/eddyxu/cpp-coveralls
[make]: https://www.gnu.org/software/make/
[boost]: http://www.boost.org/
[libgit2]: https://libgit2.github.com/
[sqlite]: https://www.sqlite.org/
[srchilite]: https://www.gnu.org/software/src-highlite/
[zlib]: http://zlib.net/
[pandoc]: http://pandoc.org/
[python]: https://www.python.org/
