NAME
====

uncov-gcov is coverage information collector for C and C++ languages that can be
used with **uncov(1)**.

**NOTE:** since uncov-gcov isn't the only way to collect C and C++ coverage for
uncov anymore, its use is discouraged.  It is provided in case it might work
better in some contexts because of some unique options (like taking exclusion
markers of LCOV into account).  It might be a good idea to switch to using
"uncov new-gcovi" subcommand.
