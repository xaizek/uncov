### Building ###

#### Defaults ####

By default debug version is build right in the source tree.

#### Targets ####

* `debug`
Builds debug version in `debug/`.

* `release`
Builds release version in `release/`.

* `sanitize-basic`
Builds debug version with undefined and address sanitizers enabled in
`sanitize-basic/`.

* `man`
Rebuilds manual page in `<out>/docs/uncov.1`, requires `pandoc`.

* `coverage`
Builds coverage in `coverage/` and commits it using `uncov` itself (which should
be installed).

* `check`
Builds and runs tests.  Combine with `debug`, `release` or `sanitize-basic` to
build and run tests in that configuration.

* `clean`
Removes build artifacts for all configurations.

* `install`
Builds the application in release mode (`release` target) and installs it.
`DESTDIR` can be set to point to root of the installation directory.

* `uninstall`
Removes executable and man page.  Do specify `DESTDIR` if it was specified on
installation.

#### Documentation ####

Rebuilding a man-page requires `pandoc` to be installed.

### Installation ###

A regular installation under `/usr/bin` can be done like this:

```
make install
```

Installation into custom directory (e.g. to make a package):

```
make DESTDIR=$PWD/build install
```

### Uninstallation ###

Similar to installation, but using `uninstall` target:

```
make uninstall
```

Or for custom location:

```
make DESTDIR=$PWD/build uninstall
```
