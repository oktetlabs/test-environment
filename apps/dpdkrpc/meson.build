# Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.

project('dpdkrpc', 'c', version: '1.0')

cc = meson.get_compiler('c')

c_args = [ '-DALLOW_EXPERIMENTAL_API' ]
add_project_arguments(c_args, language: 'c')

deps = []
link_args = []
link_depends = []

# Use dlopen/dlsym to find function addresses
link_args += [ '-rdynamic' ]
deps += [ cc.find_library('dl') ]

te_lib_dir = join_paths(get_option('te_prefix'), 'lib')

# Link whole TE libraries to pull all RPCs supports
# Typically it is done indirectly via generated symbol table which
# enforces all global functions to kept on linking, but symbol
# table is not used here yet.
link_args += [ '-Wl,--whole-archive' ]

foreach te_lib : get_option('te_libs').split(',')
    lib_a = join_paths(te_lib_dir, 'lib' + te_lib + '.a')
    link_args += [ lib_a ]
    link_depends += [ lib_a ]
endforeach

# Link whole DPDK static libraries
# Otherwise, if DPDK dependency is used, drivers, which are pulled
# using constructors, are not linked and nothing works.
foreach rte_lib : get_option('rte_libs').split(',')
    lib_a = join_paths(te_lib_dir, 'lib' + rte_lib + '.a')
    link_args += [ lib_a ]
    link_depends += [ lib_a ]
endforeach

link_args += [ '-Wl,--no-whole-archive' ]

deps += [ dependency('threads') ]
deps += [ cc.find_library('m') ]
deps += [ cc.find_library('rt') ]

dep_bsd = dependency('libbsd', required: false)
if dep_bsd.found()
    deps += [ dep_bsd ]
endif

dep_tirpc = dependency('libtirpc', required: false)
if dep_tirpc.found()
    deps += [ dep_tirpc ]
endif

# Applications are installed to TA root directory
executable('dpdkrpc',
    sources: [ 'main.c' ],
    install: true, install_dir: '',
    link_args: link_args,
    link_depends: link_depends,
    dependencies: deps)
