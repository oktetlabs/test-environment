# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2024 OKTET Labs Ltd. All rights reserved.

name = get_option('agent-proxy-name')
ta_platform = get_option('agent-proxy-platform')
tadir = join_paths(get_option('agentsdir'), name)

includes = []
link_args = []

c_args += [ '-DTE_AGT_PLATFORM="' + ta_platform + '"' ]

deps = [ dep_threads, cc.find_library('m') ]

sources = files(
    'main.c',
    'conf.c',
)

ta_symtbl_libs = [
    'tools',
    'rcfpch',
    'agentlib',
    'tad',
]

ta_non_symtbl_libs = [
    'conf_oid',
    'loggerta',
    'logger_file',
    'logger_core',
]

foreach extra_lib : get_option('agent-proxy-libs')
    if not ta_symtbl_libs.contains(extra_lib)
        ta_symtbl_libs += [ extra_lib ]
    endif
endforeach

foreach l : ta_non_symtbl_libs + ta_symtbl_libs
    deps += [ get_variable('dep_lib_static_' + l) ]
endforeach

generated_sources = []

if get_option('agent-proxy-symtbl')
    # FIXME add check for: nm --format=sysv
    nm = find_program('nm')
    ta_library = static_library('ta', sources,
                                include_directories: includes,
                                c_args: c_args,
                                dependencies: deps)
    symtbl_src = [ ta_library ]
    foreach l : ta_symtbl_libs
        symtbl_src += [ get_variable('te_lib_' + l) ]
    endforeach
    c_args += [ '-DENABLE_GENERATED_SYMTBL' ]
    symtbl = custom_target('gen-symtbl', output: 'symtbl.c', capture: true,
                           input: [ symtbl_src ],
                           command: [ te_generate_symtbl, nm, '@INPUT@' ])
    generated_sources += [ symtbl ]
endif

executable('ta', sources, generated_sources,
           install: true, install_dir: tadir,
           include_directories: includes,
           c_args: c_args,
           link_args: link_args,
           dependencies: deps)