# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2019-2022 OKTET Labs Ltd. All rights reserved.

project('sefltest', 'c',
    version : '1.0.0',
    meson_version: '>= 0.49.0',
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

# The line below would produce empty dependencies on systems
# having no tirpc - it is not a problem.
dep_tirpc = dependency('libtirpc', required: false)

test_deps += [ dep_tirpc ]

te_libs = [
    'rcfapi',
    'conf_oid',
    'confapi',
    'tapi_rpc',
    'tapi_tad',
    'tapi_env',
    'tools',
    'tapi',
    'logger_core',
    'logger_ten',
    'rcfrpc',
    'rpc_types',
    'asn',
    'ndn',
    'tapi_tool',
    'tapi_job',
    'tapi_fio',
    'tapi_dns',
    'tapi_performance',
]

foreach lib : te_libs
    test_deps += dependency('te-' + lib)
endforeach

test_deps += cc.find_library('m')

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
    'tools',
    'cs',
    'tapi',
    'rpc',
    'apps',
    'tad',
    'trc',
    'monitors',
]

mydir = package_dir
foreach package : packages
    package_dir = join_paths(mydir, package)
    package_tests_c = []
    subdir(package)
endforeach
