# Copyright (C) 2020-2022 OKTET Labs. All rights reserved.

tests = [
    'file_append',
    'file_copy',
    'file_create_delete',
    'file_directory',
    'file_get',
    'file_put',
    'file_read',
    'file_write',
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
