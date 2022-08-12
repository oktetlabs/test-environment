# This meson script is intended to be used with the TE_TA_APP macro
# after all the TE libraries and the main TE agent have been built.
# More information can be found in the TE_TA_APP macro description.
#
# Copyright (C) 2020-2022 OKTET Labs. All rights reserved.

# The restriction on the meson version is due to the use of arrays in options.
project('ta_rpcprovider', 'c', meson_version: '>= 0.44.0')

cc = meson.get_compiler('c')

#
# Get project options
#
c_args = get_option('te_cppflags')
ld_args = get_option('te_ldflags')
te_libdir = get_option('te_libdir')
te_libs = get_option('te_libs')

#
# Prepare linker arguments
#

# Use static implementation of TE libraries
ld_args += [ '-L' + te_libdir, '-Wl,-Bstatic' ]
foreach l : te_libs
    ld_args += [ '-l' + l ]
endforeach

# Use shared external (system) libraries and only if are needed
ld_args += [ '-Wl,-Bdynamic', '-Wl,--as-needed' ]

# External dependencies
deps = []
extra_deps = get_option('extra_deps')
foreach d : extra_deps
    deps += [ dependency(d) ]
endforeach

deps += [ dependency('threads', required: false) ]
deps += [ dependency('libbsd', required: false) ]

dep_rpc = dependency('libtirpc', required: false)
if dep_rpc.found()
    # Modern Linux distros, such as Fedora 28, do not have RPC
    # support in glibc: libtirpc can be used instead.
    deps += dep_rpc
else
    # Solaris2 (SunOS 5.11) -lnsl
    deps += cc.find_library('nsl', required: false)
endif

# The list of possible dependencies from external (system) libraries
ext_libs = [ 'dl', 'm', 'rt' ]
foreach l : ext_libs
    # Note: the 'static' argument is allowed starting from meson 0.51.0,
    # suppose, if found, a shared version of library is present in a system.
    deps += cc.find_library(l, required: false)
endforeach

#
# Generate symtables
#
symtbl_libs = []
foreach l : te_libs
    symtbl_libs += [ te_libdir + '/lib' + l + '.a' ]
endforeach

te_generate_symtbl = find_program('te_generate_symtbl.sh')

# FIXME: incompatible with cross-build
nm = find_program('nm')

symtbl = custom_target('gen-symtbl', output: 'symtbl.c', capture: true,
                       input: [ symtbl_libs ],
                       command: [ te_generate_symtbl, nm, '@INPUT@' ]
                      )
#
# Build rpc provider
#
executable(get_option('app-name'),
           sources: [ 'main.c', symtbl ],
           c_args: c_args,
           link_args: ld_args,
           dependencies: deps
          )
