# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2018-2022 OKTET Labs Ltd. All rights reserved.

is_multi_build_library = true

if not cc.has_function('pthread_atfork', dependencies: dep_threads)
    error('pthread_atfork is not supported')
endif

conf = configuration_data()

foreach opt : lib_opts_agentlib
    if get_variable(('opt-' + opt).underscorify())
        # substring() is not supported on meson versions < 0.56
        enable_opt = 'ENABLE_'.join(opt.split('agentlib-'))
        conf.set(enable_opt.to_upper().underscorify(), 1)
    endif
endforeach

if get_variable('opt-agentlib-power-sw'.underscorify())
    foreach h : [ 'termios.h', 'linux/ppdev.h', 'pwd.h']
        if cc.has_header(h, args: c_args)
            conf.set('HAVE_' + h.to_upper().underscorify(), 1)
        endif
    endforeach
endif

if cc.has_function('getpwnam_r')
   conf.set('HAVE_PWNAM_R', 1)
endif


agentlib_config_h = configure_file(input: 'agentlib_config.h.meson',
                                   output: 'agentlib_config.h',
                                   configuration: conf)
headers += [ agentlib_config_h ]

headers += files(
    'agentlib.h',
    'agentlib_defs.h',
)

sources += files(
    'ftp.c',
    'keys.c',
    'net.c',
    'l4_port.c',
    'power_sw.c',
    'process.c',
    'symbols.c',
    'unistd.c',
)
te_libs += [
    'tools',
    'loggerta',
]
