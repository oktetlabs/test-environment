# Copyright (C) 2018 OKTET Labs. All rights reserved.

conf = configuration_data()
conf.set_quoted('PACKAGE_STRING',
                meson.project_name() + ' ' + meson.project_version())

c_args = []
deps = [ dep_threads, dep_libxml2, dep_popt ]

te_libs = [
    'bsapi',
    'conf_oid',
    'confapi',
    'ipc',
    'ipcserver',
    'log_bufs',
    'logger_ten',
    'logic_expr',
    'tapi',
    'tools',
]

if get_option('trc')
    c_args += [ '-DWITH_TRC' ]
    te_libs += [ 'trc' ]
endif

if cc.has_type('xmlError', prefix: '#include <libxml/xmlerror.h>',
               dependencies: dep_libxml2)
    conf.set('HAVE_XMLERROR', 1)
endif

dep_curses = cc.find_library('curses', required: false)
if dep_curses.found() and cc.has_function('start_color',
                                          dependencies: dep_curses)
    conf.set('HAVE_COLOR', 1)
    deps += dep_curses
endif

dep_readline = cc.find_library('readline', required: false)
if dep_readline.found() and cc.has_function('readline',
                                            dependencies: dep_readline)
    conf.set('HAVE_READLINE', 1)
    deps += [ dep_readline ]
endif

dep_crypto = cc.find_library('crypto', required: false)
if dep_crypto.found() and cc.has_function('MD5', dependencies: dep_crypto)
    conf.set('HAVE_OPENSSL', 1)
    deps += [ dep_crypto ]
endif

configure_file(input: 'config.h.meson',
               output: 'config.h',
               configuration: conf)
configuration_inc = include_directories('.')
c_args += [ '-DHAVE_CONFIG_H' ]

sources = [
    'build.c',
    'config_parse.c',
    'config_prepare.c',
    'config_walk.c',
    'enumerate.c',
    'mix.c',
    'reqs.c',
    'run.c',
    'scenario.c',
    'term_out.c',
    'test_path.c',
    'tester.c',
    'tester_cmd_monitor.c',
    'tester_interactive.c',
    'tester_serial_thread.c',
    'type_lib.c',
    'test_msg.c',
]

sources += lex_gen.process('test_path_lex.l')
sources += gram_gen.process('test_path_gram.y')

foreach te_lib: te_libs
    deps += [ get_variable('dep_lib_' + te_lib) ]
endforeach

executable('te_tester', sources, install: true,
           include_directories: [ configuration_inc, te_include ],
           c_args: c_args,
           dependencies: deps)

# Tester tests are broken
#subdir('tests')
