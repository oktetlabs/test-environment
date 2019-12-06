# Copyright (C) 2019 OKTET Labs. All rights reserved.

project('sefltest', 'c',
    version : '1.0.0',
    meson_version: '>= 0.44.1',
)

te_path = get_option('te_path')
te_libdir = get_option('te_libdir')
add_project_arguments(get_option('te_cflags').split(), language: 'c')
add_project_arguments('-Werror', language : 'c')
add_project_link_arguments(get_option('te_ldflags').split(), language: 'c')

cc = meson.get_compiler('c')

test_deps = [ ]

te_libs = [
]

foreach lib : te_libs
    test_deps += cc.find_library(lib, dirs: te_libdir)
endforeach

tests = [
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