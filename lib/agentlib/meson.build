# Copyright (C) 2018-2019 OKTET Labs. All rights reserved.

is_multi_build_library = true

if not cc.has_function('pthread_atfork', dependencies: dep_threads)
    error('pthread_atfork is not supported')
endif

conf = configuration_data()

foreach opt : lib_opts_agentlib
    if get_variable(('opt-' + opt).underscorify())
        conf.set('ENABLE_' + opt.to_upper().underscorify(), 1)
    endif
endforeach

if get_variable('opt-agentlib-power-sw'.underscorify())
    foreach h : [ 'termios.h', 'linux/ppdev.h' ]
        if cc.has_header(h, args: c_args)
            conf.set('HAVE_' + h.to_upper().underscorify(), 1)
        endif
    endforeach
endif

agentlib_config_h = configure_file(input: 'agentlib_config.h.meson',
                                   output: 'agentlib_config.h',
                                   configuration: conf)
headers += [ agentlib_config_h ]

headers += files(
    'agentlib.h',
    'agentlib_defs.h',
    'dahdi_user.h',
    'telephony.h',
)

sources += files(
    'ftp.c',
    'net.c',
    'power_sw.c',
    'process.c',
    'symbols.c',
    'telephony.c',
    'unistd.c',
)
te_libs += [
    'tools',
    'loggerta',
]
