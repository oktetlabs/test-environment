# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2025 OKTET Labs Ltd. All rights reserved.

packages = [
    'distributed',
]

install_data([ 'package.xml' ], install_dir: package_dir)

rpc_mydir = package_dir
foreach package : packages
    package_dir = join_paths(rpc_mydir, package)
    package_tests_c = []
    subdir(package)
endforeach
