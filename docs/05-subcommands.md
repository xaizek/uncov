SUBCOMMANDS
===========

Syntax of build numbers
-----------------------

Build numbers are specified in arguments for subcommands by being prepended with
`@` sign.  So to refer to build number `5`, one would write `@5`.

Build numbers start at `1`, this leaves `@0` unused.  It is thus repurposed to
be a handy shortcut for the latest build.  An alternative form of writing `@0`
is `@@`.

Build numbers can also be specified in the form of `@-N`, in which case they
select Nth to the latest build.  For example, to specify range from previous
build to one build before that one would write `@-1 @-2`.

Lastly, branch names can be used to specify latest build from that branch (e.g.,
`@master`).

Resolving ambiguity
-------------------

Some commands can take optional build number, which opens the door for ambiguity
between file/directory names and build identifiers.  Anything that starts with
`@` at a suitable position on command-line is assumed to be build number.  For
files which have `@` as prefix, specifying build number becomes mandatory.  As
an example:

```bash
# this doesn't work
uncov show @strangely-named-file
# this is equivalent and works
uncov show @@ @strangely-named-file
```

Default build
-------------

If a subcommand accepts build number, in almost all cases it's an optional
parameter and latest build is used when this argument is omitted.

Subcommand aliases
------------------

Instead of requiring arguments for subcommands a different approach has been
taken.  Some commands have several names and depending on how you call them,
they act slightly differently.

Paths
-----

As a convenience when current working directory is under work tree of a
repository, paths that do not start with a slash `/` are automatically converted
to be relative to root of the repository.
