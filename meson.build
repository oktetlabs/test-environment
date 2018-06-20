# Copyright (C) 2018 OKTET Labs. All rights reserved.

project('test-environment', 'c',
    default_options: ['default_library=static'],
)

te_cflags = get_option('cflags').split()
te_ldflags = get_option('ldflags').split()

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
                     arguments: [ '-y', '@EXTRA_ARGS@',
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

cc = meson.get_compiler('c')

dep_threads = dependency('threads')
if get_option('engine')
    dep_libxml2 = dependency('libxml-2.0')
    dep_popt = dependency('popt')
endif

subdir('include')
subdir('lib')
if get_option('engine')
    subdir('engine')
endif
subdir('agents')
