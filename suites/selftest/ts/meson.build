# Copyright (C) 2019 OKTET Labs. All rights reserved.

project('sefltest', 'c',
    version : '1.0.0',
    meson_version: '>= 0.44.1',
    default_options: 'werror=true',
)

te_path = get_option('te_path')
te_libdir = get_option('te_libdir')
add_project_arguments(get_option('te_cflags').split(), language: 'c')
add_project_link_arguments(get_option('te_ldflags').split(), language: 'c')

te_tests_info_sh = find_program(join_paths(te_path, 'te_tests_info.sh'))

cc = meson.get_compiler('c')

test_deps = [
    dependency('threads'),
]

te_libs = [
    'rcfapi',
    'confapi',
    'tapi_rpc',
    'tapi_tad',
    'tapi_env',
    'tools',
    'tapi',
    'logger_ten',
    'rcfrpc',
    'tapi_tool',
    'tapi_upnp',
    'tapi_job',
]

foreach lib : te_libs
    test_deps += cc.find_library(lib, dirs: te_libdir)
endforeach

tests = [
    'prologue',
]

package_dir = 'ts'
package_tests_c = [ ]

foreach test : tests
    test_exe = test
    test_c = test + '.c'
    package_tests_c += [ test_c ]
    executable(test_exe, test_c, install: true, install_dir: package_dir,
               dependencies: test_deps)
endforeach

install_data([ 'package.xml' ], install_dir: package_dir)

packages = [
    'minimal',
    'cs',
    'tapi',
    'rpc',
    'apps',
]

mydir = package_dir
foreach package : packages
    package_dir = join_paths(mydir, package)
    package_tests_c = []
    subdir(package)
endforeach
