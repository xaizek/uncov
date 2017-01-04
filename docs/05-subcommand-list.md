LIST OF SUBCOMMANDS
===================

build
-----

Displays information about one build.

**Usage: build**

Describes the last build.

**Usage: build \<build\>**

Describes **\<build\>**.

builds
------

Lists builds.

**Usage: builds**

Lists at most 10 most recent builds.

**Usage: builds \<max list length\>**

Lists at most **\<max list length\>** most recent builds.

**Usage: builds all**

Lists all builds.

changed
-------

Same as **files** subcommand, but omits listing files which have their coverage
rate unchanged.

See description of **files** subcommand below for syntax.

diff
----

Compares builds, directories or files.  Lines of files are compared by their
state (i.e., changes in number of hits when both old and new values are bigger
than `0` are treated as no change).

**Usage: diff**

Compares the last build with its predecessor.

**Usage: diff \<old build\>**

Compares the last build with **\<old build\>**.

**Usage: diff \<old build\> \<new build\>**

Compares **\<new build\>** with **\<old build\>**.

**Usage: diff [\<old build\>] [\<new build\>] \<path\>**

See forms above for information about first two arguments.  If **\<path\>**
specifies directory in either of two builds, only files under it and below are
compared.  If **\<path\>** specifies file, only that file is compared.

diff-hits
---------

Same as **diff** subcommand, but considers change of number of hits of a line to
be significant change.

See description of **diff** subcommand above for syntax.

dirs
----

Lists statistics of files grouped by directories they're located in.

**Usage: dirs**

Lists all directories of the last build.

**Usage: dirs \<build\>**

Lists all directories of **\<build\>**.

**Usage: dirs \<old build\> \<new build\>**

Lists all directories of **\<new build\>** comparing them against directories in
**\<old build\>**.

**Usage: dirs [\<build\>] \<directory path\>**

Lists directories of **\<build\>** (or last build) located under
**\<directory path\>**.

**Usage: dirs [\<old build\>] [\<new build\>] \<directory path\>**

See forms above for information about first two arguments.  Lists directories
located under **\<directory path\>**.

files
-----

Lists statistics of files.

**Usage: files**

Lists all files of the last build.

**Usage: files \<build\>**

Lists all files of **\<build\>**.

**Usage: files \<old build\> \<new build\>**

Lists all files of **\<new build\>** comparing them against files in
**\<old build\>**.

**Usage: files [\<build\>] \<directory path\>**

Lists files of **\<build\>** (or last build) located under
**\<directory path\>**.

**Usage: files [\<old build\>] [\<new build\>] \<directory path\>**

See forms above for information about first two arguments.  Lists files located
under **\<directory path\>**.

get
---

Dumps coverage information of a file.

**Usage: get \<build\> \<file path\>**

Prints information about the file in this form:
```
<commit>
<line1 coverage as integer>
<line2 coverage as integer>
<line3 coverage as integer>
...
```

See description of **new** subcommand below for meaning of integer values.

missed
------

Same as **show** subcommand, but folds not relevant and covered lines and
thus displays only parts of files that lack coverage.

See description of **show** subcommand below for syntax.

new
---

Imports new build from standard input.

**Usage: new**

Reads coverage information from standard input in the following format:
```
<commit>
<branch name>
<file name relative to repository root>
<MD5 hash of file contents>
<number of lines of coverage>
<line1 coverage as integer> <line2 coverage as integer> ...
<all other files in the same format>
```

Integers have the following meaning:

 * when less than zero (specifically `-1`) -- line is not relevant;
 * when equal to zero -- line is not covered (missed);
 * when greater than zero -- line is covered and was hit that many times.

show
----

Prints whole build, files under directory or a single file with coverage
information.

**Usage: show**

Prints all files of the last build.

**Usage: show \<build\>**

Prints all files of **\<build\>**.

**Usage: show \<build\> \<path\>**

Prints files of **\<build\>** (or last build) located under **\<path\>** if it
specifies directory or one specific file.
