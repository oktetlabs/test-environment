# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2019-2022 OKTET Labs Ltd. All rights reserved.

tests = [
    'changed',
    'dir',
    'key',
    'loadavg',
    'loop',
    'num_jobs',
    'oid',
    'partial_restore',
    'pci',
    'phy',
    'process',
    'process_autorestart',
    'process_ping',
    'set_restore',
    'ts_subtree',
    'uname',
    'unused_backup',
    'user',
    'vlans',
    'vm',
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
