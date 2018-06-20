# Copyright (C) 2018 OKTET Labs. All rights reserved.

rcf_consistency_checks = custom_target('get-rcf-consistency-check',
    output: 'rcf_consistency_checks.c',
    capture: true, build_always: true,
    command: [ te_rcf_consistency_checks, '--generate-c' ])

deps = [ dep_threads, dep_libxml2, dep_popt ]

common_deps = [
    dep_lib_rcfapi,
    dep_lib_logger_ten,
    dep_lib_ipc,
    dep_lib_tools,
]

dep_dl = cc.find_library('dl', required: false)
if dep_dl.found()
    deps += [ dep_dl ]
endif

executable('te_rcf', 'rcf.c', rcf_consistency_checks, install: true,
           link_args: [  '-rdynamic', '-export-dynamic' ],
           dependencies: [ common_deps, dep_lib_ipcserver, deps ])

executable('te_rcf_shutdown', 'rcf_shutdown.c', install: true,
           dependencies: common_deps)

# Use static linkage for tool to be used by human
executable('te_rcf_tadead', 'rcf_tadead.c', install: true,
           c_args: c_args,
           link_args: [ '-static' ],
           dependencies: common_deps)
