CONFIGURATION
=============

Location and format
-------------------

Configuration is read from **\<repository-directory\>/uncov.ini** file.  If it
doesn't exist or contains invalid data (e.g., duplicated keys), default settings
remain intact.

The file has regular ini-format and can contain either comments that start with
**;** or key-value pairs like **tab-size = 2** (with or without spaces).

Values are interpreted according to types of their keys.  Keys with values that
are not convertible to corresponding types are ignored.

Available settings
------------------

Format of each entry below:

    <option> (<type>, [app:] <default value>)

    <description>

**low-bound** (floating point, 70)

Percentage boundary between low and medium coverage levels.  Normalized to be in
the [0, 100] range.  If **low-bound > hi-bound**, their values are swapped.

**hi-bound** (floating point, 90)

Percentage boundary between medium and high coverage levels.  Normalized to be
in the [0, 100] range.  If **low-bound > hi-bound**, their values are swapped.

**tab-size** (integer, 4)

Width of tabulation in spaces.

**min-fold-size** (integer, **uncov**: 3, **uncov-web**: 4)

Minimal number of lines to be folded.

**fold-context** (integer, 1)

Number of visible lines above and below changes.

**diff-show-lineno** (boolean, **uncov**: false, **uncov-web**: true)

Whether line numbers are displayed in diffs.
