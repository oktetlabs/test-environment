# Copyright (C) 2020 OKTET Labs. All rights reserved.

packages = [
    'nptcp',
    'torrent',
]

install_data([ 'package.xml' ], install_dir: package_dir)

tapi_tool_mydir = package_dir
foreach package : packages
    package_dir = join_paths(tapi_tool_mydir, package)
    package_tests_c = []
    subdir(package)
endforeach
