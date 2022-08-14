# Copyright (C) 2018-2022 OKTET Labs Ltd. All rights reserved.

gen = find_program('filter_conf.html.sh')

misc_dir = join_paths(
    get_option('datadir'),
    join_paths('rgt-format', 'xml2html', 'misc'),
)

html = custom_target(
    'html',
    output: 'filter_conf.html',
    input: ['filter_conf.html.in'],
    capture: true,
    command: [gen, '@INPUT@', '..'], # '..' is the DATADIR for filter_conf
    install: true,
    install_dir: misc_dir,
)

install_data('style.css', install_dir: misc_dir)
