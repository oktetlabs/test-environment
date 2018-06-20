# Copyright (C) 2018 OKTET Labs. All rights reserved.

subdir('tmpls')

header = custom_target(
    'header',
    output: 'rgt-filter-xml2tcl.h',
    input: tmpls,
    capture: true,
    command: [header_gen_script, '@INPUT@'],
)

rgt_filter_xml2tcl = executable(
    'rgt-filter-xml2tcl',
    'main.c',
    header,
    include_directories: inc,
    dependencies: [dep_glib, dep_libxml2, dep_popt],
    c_args: c_args,
    link_with: libtmpls,
    install: true,
)
