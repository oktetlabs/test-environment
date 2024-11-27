# Copyright (C) 2025 OKTET Labs. All rights reserved.

tests = [
    'test_exact',
    'test_intersect',
    'test_mixed',
    'test_new',
    'test_override',
    'test_subset',
    'test_superset',
]

packages = [
    'iterated_package',
    'new_package',
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

rpc_mydir = package_dir
foreach package : packages
    package_dir = join_paths(rpc_mydir, package)
    package_tests_c = []
    subdir(package)
endforeach
