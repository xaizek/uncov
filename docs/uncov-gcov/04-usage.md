USAGE
=====

uncov-gcov can be used to generate coverage, but it seems to not play well with
out-of-tree builds (some coverage is missing, this issue is inherited from its
origin), so the recommended way of recording coverage information is as follows:

```
# reset coverage counters from previous runs
find . -name '*.gcda' -delete

# run tests here with something like `make check`

# generage coverage for every object file found (change "." to build root)
find . -name '*.o' -exec gcov -p {} +

# generage and combine coverage reports (--capture-worktree automatically
# makes stray commit if repository is dirty)
uncov-gcov --root . --no-gcov --capture-worktree --exclude tests | uncov new

# remove coverage reports
find . -name '*.gcov' -delete
```

These commands can be put in a separate script or embedded directly into build
system.
