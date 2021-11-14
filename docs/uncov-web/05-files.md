FILES
=====

**\<data-directory\>** in the following is either git-directory for a worktree
(see **git-worktree**(1)) or for the repository that owns it, whichever has
either of those files when checking directories in the order they are mentioned.
If no files found, repository's git-directory is used.

**\<data-directory\>/uncov.sqlite** -- storage of coverage data.

**\<data-directory\>/uncov.ini** -- configuration.
