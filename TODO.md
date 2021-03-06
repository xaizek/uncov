# Core #

## Determine build predecessor by commits and branches. ##

| ID   |  Status   |  Type         |
|------|-----------|---------------|
| ARY  |  planned  |  improvement  |

Currently it's just a mapping of `N` to `N - 1`.

The implementation could be as follows:

1. Collect commit ids from all previous builds (with `id < N`). (This is cheap.)
   These must be ordered by branch matching branch of current build and then by
   build ids.
2. Do breadth first search from commit of current build.  Pseudo-code:

        knownIdsToBuildIdsMap;
        if (knownIdsToBuildIdsMap.empty()) {
            return {};
        }

        ids = { <current build's commit> };
        while (!ids.empty()) {
            newIds = {};
            for (id : ids) {
                for (parent : id.parents()) {
                    if (parent in knownIdsToBuildIdsMap) {
                        return knownIdsToBuildIdsMap[parent];
                    }

                    newIds.insert(parent);
                }
            }
            ids = newIds;
        }
        return {};


We might want to actually find *the closest* build instead of the first one,
this should give better results.  This can be even slower, so a persistent cache
could be needed.

This can be somewhat slow, so some caching could be useful.
For that we could use memoization in a function like `getCommitParents()` that
would remember its results.

Functions to use:
 - `git_commit_parent_id()`
 - `git_commit_parentcount()`

## Consider different path sorting. ##

| ID   |  Status     |  Type    |
|------|-------------|----------|
| BSY  |  undecided  |  change  |

The one which puts contents of child above contents of current one.

## Maybe display commit information in `build`. ##

| ID   |  Status     |  Type      |
|------|-------------|------------|
| GSY  |  undecided  |  addition  |

## Maybe indicate when build matches repo reference. ##

| ID   |  Status     |  Type  |
|------|-------------|--------|
| JRY  |  undecided  |  idea  |

That is when `HEAD` of non-bare repositories matches commit of builds or
maybe even when branch ref matches build commit.

Maybe should also indicate "temporary" commits somehow.

## Source-highlight hangs if passed in std::istream is at EOF? ##

| ID   |  Status     |  Type   |
|------|-------------|---------|
| MRY  |  undecided  |  issue  |

Need to check on their trunk and see if happens there (should do nothing
instead).

## Maybe make use of information about file contents change. ##

| ID   |  Status     |  Type  |
|------|-------------|--------|
| NRY  |  undecided  |  idea  |

Could:
 * Mark somehow files that are changed between builds (changed by contents).
 * Could list them separately or just indicate as changed.

Coveralls has four listings:
 * All
 * Changed
 * Source Changed
 * Coverage Changed
Not all seem to be awfully useful though.

## Introduce DBException and RepositoryException classes? ##

| ID   |  Status     |  Type         |
|------|-------------|---------------|
| PRY  |  undecided  |  improvement  |

This can help make some error messages better, but is it really that useful
(not sure handling of exceptions will actually differ, so maybe only to
provide more details about the error).

## Maybe something to exclude files. ##

| ID   |  Status     |  Type      |
|------|-------------|------------|
| VRY  |  undecided  |  addition  |

Or should this be handled by coverage providers?

## Named pipes can be employed to pass multiple files to pager. ##

| ID   |  Status     |  Type  |
|------|-------------|--------|
| WRY  |  undecided  |  idea  |

## A way to disable file highlighting. ##

| ID   |  Status   |  Type         |
|------|-----------|---------------|
| YRY  |  planned  |  improvement  |

It's slow for huge diffs/files (quite understandable).

## Language detection for file highlight relies on file name only. ##

| ID   |  Status     |  Type   |
|------|-------------|---------|
| aRY  |  undecided  |  issue  |

Could examine for shebang or even employ `libmagic`.

## Shell completion. ##

| ID   |  Status   |  Type      |
|------|-----------|------------|
| hSY  |  planned  |  addition  |

## Maybe introduce range syntax to specify two builds. ##

| ID   |  Status     |  Type  |
|------|-------------|--------|
| kRY  |  undecided  |  idea  |

E.g. `@10..@@`.

## Maybe differentiate between ##### and ===== in gcov output. ##

| ID   |  Status     |  Type         |
|------|-------------|---------------|
| nRY  |  undecided  |  improvement  |

`#####` means not reached on normal path.

`=====` is the same, but for exceptional paths (inside `catch` branches).

## Consider displaying line numbers in "diff". ##

| ID   |  Status     |  Type    |
|------|-------------|----------|
| qRY  |  undecided  |  change  |

They are sometimes useful, but not always (and take up space).

So maybe add, but make it configurable.

## "branch" subcommand to show builds of only one branch. ##

| ID   |  Status     |  Type      |
|------|-------------|------------|
| tRY  |  undecided  |  addition  |

Or extend `builds` accordingly.

## Consider basic history editing. ##

| ID   |  Status     |  Type  |
|------|-------------|--------|
| uRY  |  undecided  |  idea  |

Maybe add an option to `new` or separate command (e.g. `amend`) or a way to
edit build history.  This can make it easier to see new coverage while
developing and then discarding that temporary data or to drop accidental/broken
builds.

Maybe just allow at most one build from working tree.

Maybe store uncommitted files in the database directly.  Stray commit is
created now by `uncov-gcov`.

Removed/deleted builds could just be marked as such to do not affect numbering.

## More flexible way to specify database location. ##

| ID   |  Status     |  Type         |
|------|-------------|---------------|
| uSY  |  undecided  |  improvement  |

For more complicated (but rarer) case when coverage repository differs from
the working one.

Symlinks work fine at the moment.

## More control over not relevant files. ##

| ID   |  Status     |  Type         |
|------|-------------|---------------|
| vSY  |  undecided  |  improvement  |

E.g., an option to don't show files with no relevant lines in output of, say,
`files`.  Or maybe just a subcommand, like `relevant`?

Maybe a way to sort files which will group such files on one part of the table.

## Think about adding titles to console output. ##

| ID   |  Status     |  Type         |
|------|-------------|---------------|
| wSY  |  undecided  |  improvement  |

The reason why it might be useful is this:

 * `uncov diff @master`
    -- diffs current build with last build on @master branch
 * `uncov changed @master`
    -- displays what was changed in build on @master branch

Syntax addresses most common use cases, however it's confusing and a title
for `changed` saying what it's only one build would be helpful.

## Maybe print distance between builds (in commits). ##

| ID   |  Status     |  Type      |
|------|-------------|------------|
| xRY  |  undecided  |  addition  |

Related to `ARY`, we need to analyze commit graph to do this.

# Vim Plugin #

## A way to populate location list with covered lines. ##

| ID   |  Status   |  Type         |
|------|-----------|---------------|
| CSY  |  planned  |  improvement  |

This is useful for source files which have almost none coverage and you want to
see what is covered.

## Bad buffer name for multiple buffers of the same file. ##

| ID   |  Status   |  Type   |
|------|-----------|---------|
| SSY  |  planned  |  issue  |

Vim doesn't allow several buffers to have the same name, so with current naming
scheme only one buffer will get the name while the other one will be
"[No Name]".

Need to identify buffers more uniquely.

## Don't depend on fugitive in the plugin. ##

| ID   |  Status   |  Type         |
|------|-----------|---------------|
| cRY  |  planned  |  improvement  |

We need too little from it.

Outline of possible implementation:

    function! s:GetGitRelPath()
        " find .git file/directory by searching up directory tree
        " print meaningful error if not in git repository
        " make absolute path of location of .git directory
        " make absolute path of current buffer
        " remove path to .git from path of the buffer
        " return the result
    endfunction

## Don't rely on autochdir in the plugin. ##

| ID   |  Status   |  Type         |
|------|-----------|---------------|
| eRY  |  planned  |  improvement  |

# Web-UI #

## For web-ui we might want to cache highlighting results. ##

| ID   |  Status     |  Type         |
|------|-------------|---------------|
| mRY  |  undecided  |  improvement  |

Highlighting is a slow process due to use of regular expressions.

## Support multiple projects in web-ui. ##

| ID   |  Status   |  Type     |
|------|-----------|-----------|
| sSY  |  planned  |  feature  |

If only one project is specified (on command-line), do as now.

If there is more than one project, then:
 * add menu item `All Projects`
 * display list of projects by default (in site root)

In code this would take a structure like this one:

    struct Project
    {
        Repository *const repo;
        BuildHistory *const bh;
    };

With global state being:

    std::map<std::string, Project> projects;
