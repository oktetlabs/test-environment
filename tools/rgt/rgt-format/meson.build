# Copyright (C) 2018 OKTET Labs. All rights reserved.

subdir('xml2html')
subdir('xml2text')
subdir('xml2html-multi')

libcapture = static_library(
    'libcapture',
    ['capture.c', 'capture.h'],
    include_directories: inc,
    c_args: c_args,
    dependencies: [dep_libxml2, dep_glib],
    link_with: libtmpls,
)

libxml2fmt_core = static_library(
    'libxml2fmt_core',
    'xml2fmt_core.c',
    include_directories: inc,
    c_args: c_args,
    dependencies: [dep_libxml2, dep_glib, dep_popt],
)


common_util_path = join_paths('..', get_option('datadir'), 'rgt-format')

rgt_xml2_tools = [
    ['xml2text', xml2text_tmpls],
    ['xml2html', xml2html_tmpls],
    ['xml2html-multi', xml2html_multi_tmpls],
]

foreach tool: rgt_xml2_tools
    header = custom_target(
        tool.get(0) + '_header',
        output: tool.get(0) + '.h',
        input: [tool.get(1)],
        capture: true,
        command: [
            header_gen_script,
            '@INPUT@',
        ],
    )
    executable(
        'rgt-' + tool.get(0),
        files(join_paths(tool.get(0), tool.get(0) + '.c'), 'capture.h'),
        header,
        include_directories: inc,
        dependencies: [dep_libxml2, dep_glib],
        c_args: c_args,
        link_with: [libtmpls, libcapture, libxml2fmt_core],
        install: true,
    )
endforeach
