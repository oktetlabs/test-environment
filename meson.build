# Copyright (C) 2018-2022 OKTET Labs. All rights reserved.

project('test-environment', 'c',
    default_options: ['default_library=shared'],
    meson_version: '>= 0.45.1'
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

dep_threads = dependency('threads')
if get_option('engine')
    dep_libxml2 = dependency('libxml-2.0')
    dep_popt = dependency('popt', required : false)
    if not dep_popt.found()
        dep_popt = cc.find_library('popt')
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
