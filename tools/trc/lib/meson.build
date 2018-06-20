# Copyright (C) 2018 OKTET Labs. All rights reserved.

common_sources = files(
    'gen_wilds.c',
    'html.c',
    're_subst.c',
    'report.c',
    'tags.c',
    'tools.c',
    'update.c',
)

libtrc_diff_sources = files(
    'diff.c',
    'diff_html.c',
    'diff_tags.c',
    'log_parse.c',
)

libtrc_report = static_library(
    'libtrc_report',
    ['log_parse.c', 'report_html.c', common_sources],
    dependencies: [dep_common],
    c_args: c_args,
)

libtrc_diff = static_library(
    'libtrc_diff',
    [libtrc_diff_sources, common_sources],
    dependencies : [dep_common],
    c_args: c_args,
)

install_data(
    ['jquery_include.sh', 'jquery_flot.js'],
    install_dir: get_option('bindir'),
)
