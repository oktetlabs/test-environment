# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2019-2022 OKTET Labs Ltd. All rights reserved.

packages = [
    'memory',
    'server',
    'unistd',
]

install_data([ 'package.xml' ], install_dir: package_dir)

rpc_mydir = package_dir
foreach package : packages
    package_dir = join_paths(rpc_mydir, package)
    package_tests_c = []
    subdir(package)
endforeach
