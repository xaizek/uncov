OPTIONS
=======

**--help, -h**
--------------

Displays short usage help.

**--version, -v**
-----------------

Displays version information.

**--verbose**
-------------

Print verbose messages.

**--dryrun**
------------

Run the script without printing report.

**--gcov** [=gcov]
------------------

Set the location of gcov.

**--gcov-options** [=""]
------------------------

Set the options given to gcov.

**-r**, **--root** [=.]
-----------------------

Set the root directory.

**-b**, **--build-root** [={discovered}]
----------------------------------------

Set the directory from which gcov will be called; by default gcov is run in the
directory of the .o files; however the paths of the sources are often relative
to the directory from which the compiler was run and these relative paths are
saved in the .o file; when this happens, gcov needs to run in the same directory
as the compiler in order to find the source files.

**--collect-root** [={value of --root}]
---------------------------------------

Directory to look gcov files in.

**-e**, **--exclude** [=""]
---------------------------

Set exclude file or directory.

**-i**, **--include** [=""]
---------------------------

Set include file or directory.

**-E**, **--exclude-pattern** [=""]
-----------------------------------

Set exclude file/directory pattern.

**-x**, **--extension** [=.h,.hh,.hpp,.hxx,.c,.cc,.cpp,.cxx,.m,.mm]
-------------------------------------------------------------------

Set extension of files to process.

**-n**, **--no-gcov**
---------------------

Do not run gcov.

**--encodings** [=utf-8,latin-1]
--------------------------------

Source encodings to try in order of preference.

**--dump** \<file\>
-------------------

Dump JSON payload to a file.

**--follow-symlinks**
---------------------

Follow symlinks.

**-c**, **--capture-worktree**
------------------------------

Make a dangling commit if working directory is dirty.

**--ref-name** [={discovered}]
------------------------------

Force custom ref name.

**--cpp-dtor-invocations**
--------------------------

Count coverage for C++ destructor invocations, which tends to show up at lines
that have the closing brace (`{`).
