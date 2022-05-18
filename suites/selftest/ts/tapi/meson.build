# Copyright (C) 2020 OKTET Labs. All rights reserved.

packages = [
    'cache',
    'job',
    'jobopt',
    'tool',
]

install_data([ 'package.xml' ], install_dir: package_dir)

tapi_mydir = package_dir
foreach package : packages
    package_dir = join_paths(tapi_mydir, package)
    package_tests_c = []
    subdir(package)
endforeach
