# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2022-2023 OKTET Labs Ltd. All rights reserved.

tests = [
    'alloc',
    'base64',
    'compare_bufs',
    'enum_map',
    'expand',
    'expand_filters',
    'expand_list',
    'extract_glob',
    'file',
    'file_chunked',
    'hexdump',
    'intset',
    'json',
    'kvpair',
    'lines',
    'make_bufs',
    'readlink',
    'rings',
    'resolvepath',
    'scalar_types',
    'scandir',
    'str_compare_versions',
    'string',
    'strpbrk_balanced',
    'units',
    'uri',
    'vector',
    'vector_mem',
    'vector_search',
]

foreach test : tests
    test_exe = test
    test_c = test + '.c'
    package_tests_c += [ test_c ]
    executable(test_exe, test_c, install: true, install_dir: package_dir,
               dependencies: test_deps)
endforeach

tests_info_xml = custom_target(package_dir.underscorify() + 'tests-info-xml',
                               install: true, install_dir: package_dir,
                               input: package_tests_c,
                               output: 'tests-info.xml', capture: true,
                               command: [ te_tests_info_sh,
                                          meson.current_source_dir() ])

install_data([ 'package.xml' ], install_dir: package_dir)

packages = [
    'timer',
    'trees',
]

tools_mydir = package_dir
foreach package : packages
    package_dir = join_paths(tools_mydir, package)
    package_tests_c = []
    subdir(package)
endforeach
