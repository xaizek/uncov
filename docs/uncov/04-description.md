DESCRIPTION
===========

`uncov` operates a number of entities, which are directly linked to subcommands
and information they output.  Below you can find their description.

Repository
----------

Repository is used as a source of file contents and its root directory (not
worktree one) is used to store data of `uncov` itself.  While the relation by
default is of one-to-one kind, same `uncov` data can be shared by multiple
copies of the same repository by linking or copying `uncov.sqlite` file
manually.

Repository that corresponds to current working directory is discovered
automatically and doesn't need to be specified explicitly.  Otherwise
**\<repo-path\>** can point either to repository directory (`.git`) or its
main worktree (doesn't seem to work with secondary worktrees).

Builds
------

The largest entity `uncov` operates within repository is a build.  A build has
the following properties:

 * number identifying the build (greater than `0`);
 * name of reference within repository (branch usually);
 * commit object that corresponds to the build;
 * date and time at which it was imported;
 * finally, set of files with their coverage that constitute a build.

Files
-----

File is the second most important thing after a build.  It's characterised by:

 * path within repository;
 * coverage information (basically an array specifying which lines are covered);
 * MD5 hash (to verify that `uncov` state is consistent with repository).

Directories
-----------

As we have set of files with their paths, we can derive interesting part of
file-system structure of repository (ignoring the rest of it, where there are no
source files).  Directory is just a sum of information of all files it contains.

Build relation
--------------

In order to be able to calculate change of coverage an ordering of builds is
imposed, which currently simply assumes that for build number `N` there is a
*previous build* with number `N - 1` (when `N > 0`, build number `0` has no
previous build).

Statistics
----------

File coverage information is the sole source of statistics.  Based on data
provided any line of code is classified as either *relevant* or *not relevant*.
Relevant lines in turn can be *covered* or *missed*.  So each source line must
be in one of three states:

 * not relevant;
 * not covered (with number of hits being `0`);
 * covered (with number of hits being greater than `0`).

Coverage rate is defined simply as number of covered lines divided by number of
relevant lines.  If file consists solely of not relevant lines (which is also
the case for files that don't exist in one of builds being compared), it's
assumed to have 100% coverage.  We also have number of lines covered, missed and
total number of relevant lines (sum of previous two) and can calculate their
changes.

It is these statistics that are displayed by subcommands alongside builds,
directories and files.  Data describing changes is calculated from state of
files in two builds: some build and build that is considered to be its
predecessor.

Comparison
----------

Comparison accounts for hierarchy: build comparison compares all their files,
directory comparison compares files under specified path, file comparison
compares only one file.

However, comparing coverage is not exactly the same as comparing files.  While
we are interested in whether changed code is covered we don't really care about
addition or removal of lines that aren't relevant for coverage.  So these
uninteresting changes are not shown.

On the other hand, change of coverage when only number of line hits increased or
decreased is also irrelevant in most cases and are shown only by separate
command.  Regular comparison draws attention mainly to lines that have changed
their state (e.g., from "not covered" to "covered" or vice versa).  Should such
lines be part of diff context, they are displayed as somewhat dimmed compared to
lines with interesting changes.

Notations
---------

For the sake of brevity interface uses several intuitive abbreviations:

 * Cov -- coverage;
 * Ref -- reference (of VCS);
 * C -- covered;
 * M -- missed;
 * R -- relevant.
