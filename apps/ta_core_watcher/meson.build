# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2022 OKTET Labs Ltd. All rights reserved.
#
# This meson script is intended to be used with the TE_TA_APP macro
# after all the TE libraries and the main TE agent have been built.
# More information can be found in the TE_TA_APP macro description.

# The restriction on the meson version is due to the use of arrays in options.
project('ta_core_watcher', 'c', meson_version: '>= 0.44.0')

cc = meson.get_compiler('c')

#
# Get project options
#
c_args = get_option('te_cppflags')
ld_args = get_option('te_ldflags')
te_libdir = get_option('te_libdir')

#
# Prepare linker arguments
#

# Use static implementation of TE libraries
ld_args += [ '-L' + te_libdir, '-Wl,-Bstatic' ]
ld_args += [ '-ltools', '-llogger_core', '-llogger_file' ]

# Use shared external (system) libraries and only if are needed
ld_args += [ '-Wl,-Bdynamic', '-Wl,--as-needed' ]

#
# Linking with libbsd may be required for string functions
# strlcat(), strlcpy() used in lib/tools/te_str.c
#
deps = [ dependency('libbsd', required: false) ]

#
# Build the application
#
executable('ta_core_watcher',
           sources: [ 'main.c' ],
           c_args: c_args,
           link_args: ld_args,
           dependencies: deps)
