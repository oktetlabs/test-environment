# Copyright (C) 2019 OKTET Labs. All rights reserved.

packages = [
    'cache',
    'upnp',
]

install_data([ 'package.xml' ], install_dir: package_dir)

tapi_mydir = package_dir
foreach package : packages
    package_dir = join_paths(tapi_mydir, package)
    package_tests_c = []
    subdir(package)
endforeach