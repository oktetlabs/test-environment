# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2018-2022 OKTET Labs Ltd. All rights reserved.

project('test-environment', 'c',
    default_options: ['default_library=shared'],
    meson_version: '>= 0.49.0'
)

# join_paths() drops all segments before absolute one and prefix given as absolute
platform_install_dir = get_option('workspace') + '/destdir' + get_option('prefix')
platform_include_dir = join_paths(platform_install_dir, 'include')
platform_lib_dir = join_paths(platform_install_dir, 'lib')

te_cflags = [ '-I' + platform_include_dir ] + get_option('cflags').split()
te_ldflags = [ '-L' + platform_lib_dir ] + get_option('ldflags').split()

add_project_arguments(te_cflags, language: 'c')

# Options like -m32 are passed via CFLAGS but should be passed to linker as well
add_project_link_arguments(te_cflags, language: 'c')
add_project_link_arguments(te_ldflags, language: 'c')

if get_option('engine')
    scripts = [
        'dispatcher.sh',
        'common_vars.sh',
    ]
    install_data(scripts, install_dir: 'bin')
endif

install_dev = get_option('install-dev') or get_option('engine')

lexer = find_program('flex')
lex_gen = generator(lexer,
                    output: [ '@BASENAME@.c', '@BASENAME@.h' ],
                    arguments: [ '@EXTRA_ARGS@',
                                 '--header-file=@OUTPUT1@',
                                 '-o', '@OUTPUT0@',
                                 '@INPUT@' ])

bison = find_program('bison')
gram_gen = generator(bison,
                     output: [ '@BASENAME@.c', '@BASENAME@.h' ],
                     arguments: [ '-y', '-Wno-yacc', '-Wno-deprecated',
                                  '@EXTRA_ARGS@',
                                  '--defines=@OUTPUT1@',
                                  '--output=@OUTPUT0@',
                                  '@INPUT@' ])

m4 = find_program('m4')
te_rpcgen = find_program('engine/builder/te_rpcgen')
rpcgen_m4 = files('include/rpcgen.m4')

gen_c_rpch_c = generator(m4, capture: true, output: [ '@BASENAME@.c' ],
                         arguments: [ '-D__SOURCE__=@INPUT@',
                                      '-P',
                                      '@SOURCE_DIR@/include/rpcconv.m4',
                                      '@INPUT@' ])

te_rcf_consistency_checks = find_program('engine/builder/te_rcf_consistency_checks')

te_generate_symtbl = find_program('engine/builder/te_generate_symtbl.sh')

cc = meson.get_compiler('c')

# Packages names (Debian/Ubuntu and Fedora/RHEL) for necessary dependencies
deps_names_deb = {
    'expect': 'expect-dev',
    'glib-2.0': 'libglib2.0-dev',
    'jansson': 'libjansson-dev',
    'libcurl': 'libcurl4-openssl-dev',
    'libnl-3.0': 'libnl-3-dev',
    'libpcre': 'libpcre3-dev',
    'libtirpc': 'libtirpc-dev',
    'libxml-2.0': 'libxml2-dev',
    'openssl': 'libssl-dev',
    'pcap': 'libpcap-dev',
    'popt': 'libpopt-dev',
    'yaml-0.1': 'libyaml-dev',
    'libcyaml': 'libcyaml-dev'
}
deps_names_rpm = {
    'expect': 'expect-devel',
    'glib-2.0': 'glib2-devel',
    'jansson': 'jansson-devel',
    'libcurl': 'libcurl-devel',
    'libnl-3.0': 'libnl3-devel',
    'libpcre': 'pcre-devel',
    'libtirpc': 'libtirpc-devel',
    'libxml-2.0': 'libxml2-devel',
    'openssl': 'openssl-devel',
    'pcap': 'libpcap-devel',
    'popt': 'popt-devel',
    'yaml-0.1': 'libyaml-devel',
    'libcyaml': 'libcyaml-devel'
}

missed_deps = []
required_deps = []

dep_threads = dependency('threads')
if get_option('engine')
    dep_libxml2 = dependency('libxml-2.0', required: false)
    required_deps += 'libxml-2.0'
    if not dep_libxml2.found()
        missed_deps += 'libxml-2.0'
    endif
    dep_popt = dependency('popt', required : false)
    required_deps += 'popt'
    if not dep_popt.found()
        dep_popt = cc.find_library('popt', required: false)
        if not dep_popt.found()
            missed_deps += 'popt'
        endif
    endif
endif

if get_option('engine')
    subdir('doc')
endif
subdir('include')
subdir('lib')
subdir('engine')
subdir('agents')
subdir('tools')

printed_deps = []

has_missed_pkgs_names = false
foreach dep : required_deps
    if dep not in printed_deps
        if not deps_names_deb.has_key(dep)
            warning('No deb package name for required dependency', dep)
            has_missed_pkgs_names = true
        endif
        if not deps_names_rpm.has_key(dep)
            warning('No rpm package name for required dependency', dep)
            has_missed_pkgs_names = true
        endif
        printed_deps += dep
    endif
endforeach

if has_missed_pkgs_names
    error('Missed packages names for required dependencies')
endif

printed_deps = []

if missed_deps.length() != 0
    missed_deps_str_deb = 'Debian/Ubuntu:'
    missed_deps_str_rpm = 'Fedora/RHEL:'

    foreach dep : missed_deps
        if dep not in printed_deps
            missed_deps_str_deb += ' ' + deps_names_deb.get(dep)
            missed_deps_str_rpm += ' ' + deps_names_rpm.get(dep)
            printed_deps += dep
        endif
    endforeach

    warning('Install:')
    warning(missed_deps_str_deb)
    warning(missed_deps_str_rpm)
    error('Missed required dependencies')
endif
