# Copyright (C) 2018 OKTET Labs. All rights reserved.

data = files(
    'filter_conf.html',
    'ftiens4.js',
    'lframe_style.css',
    'log_style.css',
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
