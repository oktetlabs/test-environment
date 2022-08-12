# Copyright (C) 2018-2022 OKTET Labs. All rights reserved.

common_sources = ['rgt_log_bundle_common.c', 'rgt_log_bundle_common.h']
common_libs = declare_dependency(
    dependencies: [dep_lib_tools, dep_lib_logger_file, dep_lib_logger_core],
)

rgt_log_bundle = [
    'rgt-caps-recover',
    'rgt-log-split',
    'rgt-log-merge',
    'rgt-log-recover',
]

foreach tool: rgt_log_bundle
    executable(
        tool,
        [tool.underscorify() + '.c', common_sources],
        include_directories: inc,
        dependencies: [dep_popt, common_libs],
        install: true,
    )
endforeach

install_data(
    [
        'rgt-log-bundle-create',
        'rgt-log-bundle-get-original',
        'rgt-log-bundle-get-item',
        'te_pixz_wrapper',
        'te_repack_raw_logs',
    ],
    install_dir: get_option('bindir'),
)
