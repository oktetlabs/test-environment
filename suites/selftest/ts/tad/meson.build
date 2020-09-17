# Copyright (C) 2020 OKTET Labs. All rights reserved

tests = [
    'tad_prologue',
]

package_tests_c = [ ]

foreach test : tests
    test_exe = test
    test_c = test + '.c'
    package_tests_c += [ test_c ]
    executable(test_exe, test_c, install: true, install_dir: package_dir,
               dependencies: test_deps)
endforeach

packages = [
    'atm',
    'common',
    'eth',
    'ipstack',
]

install_data([ 'package.xml' ], install_dir: package_dir)

tad_mydir = package_dir
foreach package : packages
    package_dir = join_paths(tad_mydir, package)
    package_tests_c = []
    subdir(package)
endforeach
