# Copyright (C) 2018 OKTET Labs. All rights reserved.

project('test-environment', 'c',
    default_options: ['default_library=static'],
)

te_cflags = get_option('cflags').split()
te_ldflags = get_option('ldflags').split()

add_project_arguments(te_cflags, language: 'c')

# Options like -m32 are passed via CFLAGS but should be passed to linker as well
add_project_link_arguments(te_cflags, language: 'c')
add_project_link_arguments(te_ldflags, language: 'c')

if get_option('engine')
    scripts = [
        'dispatcher.sh',
        'common_vars.sh',
    ]
    install_data(scripts, install_dir: 'bin')
endif

cc = meson.get_compiler('c')
