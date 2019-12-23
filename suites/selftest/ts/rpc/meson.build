# Copyright (C) 2019 OKTET Labs. All rights reserved.

packages = [
    'server',
]

install_data([ 'package.xml' ], install_dir: package_dir)

rpc_mydir = package_dir
foreach package : packages
    package_dir = join_paths(rpc_mydir, package)
    package_tests_c = []
    subdir(package)
endforeach