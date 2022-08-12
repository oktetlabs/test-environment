# Copyright (C) 2018-2022 OKTET Labs. All rights reserved.

data = files(
    'bootstrap.min.css',
    'bootstrap.min.js',
    'jquery.min.js',
    'Chart.bundle.min.js',
    'Chart.min.css',
    'chartjs-plugin-zoom.js',
    'filter_conf.html',
    'ftiens4.js',
    'lframe_style.css',
    'hammer.min.js',
    'log_filter.js',
    'log_style.css',
    'renderjson.js',
    'right_frame.html',
    'style.css',
    'ua.js',
)

install_data(
    data,
    install_dir: join_paths(
        get_option('datadir'),
        join_paths('rgt-format', 'xml2html-multi', 'misc'),
    ),
)
