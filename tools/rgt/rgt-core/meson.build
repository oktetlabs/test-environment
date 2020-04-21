# Copyright (C) 2018 OKTET Labs. All rights reserved.

rgt_core_sources = files(
    'filter.c',
    'flow_tree.c',
    'index_mode.c',
    'io.c',
    'junit_mode.c',
    'live_mode.c',
    'log_format.c',
    'log_format_v1.c',
    'log_msg.c',
    'memory.c',
    'postponed_mode.c',
    'rgt_core.c',
)

rgt_core = executable(
    'rgt-core',
    rgt_core_sources,
    include_directories: inc,
    dependencies: [dep_glib, dep_popt, dep_libxml2, dep_lib_tools,
                   dep_lib_logger_core, dependency('jansson'),
                   dep_lib_log_proc],
    install: true,
    c_args: c_args,
)

rgt_conv = custom_target(
    'rgt-conv',
    output: 'rgt-conv',
    input: ['rgt-conv.in'],
    capture: true,
    command: [
        'sed',
        '-e', 's,@SHELL@,/bin/bash,g',
        '-e', 's,@PACKAGE@,' + package + ',g',
        '-e', 's,@VERSION@,' + version + ',g',
        '-e', 's,@TE_COPYRIGHT@,' + te_copyright + ',g',
        '@INPUT@',
    ],
    install: true,
    install_dir: get_option('bindir'),
)

# The right way to change permission of file would be
# passing install_mode argument to custom_target
# but it is available only since version 0.47.0 of meson
# We cannot require the version right now since it is not available
# in many distros so we use this ugly method

meson.add_install_script(
    'chmod-installed-file.sh', 'a+x',
    join_paths(get_option('bindir'), 'rgt-conv'),
)
