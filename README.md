[SPDX-License-Identifier: Apache-2.0]::
[Copyright (C) 2004-2023 OKTET Labs Ltd. All rights reserved.]::

# OKTET Labs. Test Environment

OKTET Labs Test Environment (TE) is a software product that is intended to
ease creating automated test suites.

## Table of Contents

- [OKTET Labs. Test Environment](#oktet-labs-test-environment)
  - [Table of Contents](#table-of-contents)
  - [Documentation](#documentation)
  - [Build](#build)
    - [Briefly](#briefly)
    - [Dependencies](#dependencies)
    - [External libraries](#external-libraries)
  - [BASH-completion](#bash-completion)

## Documentation

TE has two types of documentation:

1. Auto-generated Doxygen documentation is built using the

   ```sh
   ./gen_doxygen
   ```

   script located in the top directory. Most versions of Doxygen are supported.

   The documentation can be found at `doc/generated/html/`.

   Doxygen warnings can be found in the `./doxygen.warn` file.

   Some pictures require `ditaa` to be installed.

2. Static documents can be found in the `doc/` folder.
   However, these may be outdated, so it is recommended to check
   the Doxygen documentation first.

## Build

Details of building TE can be found in the generated documentation.

First thing to do:

```sh
export TE_BASE=<TE SOURCES DIR>
```

### Briefly

- To build standalone TE (without an external test suite), run:

  ```sh
  ./dispatcher.sh
  ```

  and wait until the build is complete.

- To build a test suite, navigate to the test suite directory and execute:

  ```sh
  ./run.sh
  ```

  This script will perform the same actions as `dispatcher.sh`, but it will also
  build the test suite libraries and tests.

### Dependencies

Dependencies are detailed in the TE build and test suite documentation.

### External libraries

External libraries can be specified using the `TE_EXT_LIBS` macro in
`builder.conf` (see comments in `engine/builder/builder.m4`).

The `TE_EXT_LIB` environment variable should be set to
`http://oktetlabs.ru/~tester/te`.

External libraries should be placed in
`http://oktetlabs.ru/~tester/te/<platform>`, e.g.,
`http://oktetlabs.ru/~tester/te/i686-pc-linux-gnu`.

Libraries should be `*.tgz` archives that contain the `lib/` and `include/`
directories. They are simply unpacked to the installation directory of the
corresponding platform.

## BASH-completion

If you want to use BASH-completion with TE scripts, add the following line to
your `~/.bash_completion` file (or any appropriate place):

```sh
complete -F _configure_func $default ./dispatcher.sh ./run.sh
```
