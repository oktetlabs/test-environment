# Copyright (C) 2018 OKTET Labs. All rights reserved.

if not cc.has_function('pthread_atfork', dependencies: dep_threads)
    error('pthread_atfork is not supported')
endif

conf = configuration_data()

opts = [
    'dlfcn-lookup',
    'ftp',
    'power-sw',
    'telephony',
    'upnp',
]

foreach opt : opts
    if get_option('agentlib-' + opt)
        conf.set('ENABLE_' + opt.to_upper().underscorify(), 1)
    endif
endforeach

if get_option('agentlib-power-sw')
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
)
deps += [
    dep_lib_tools,
    dep_lib_loggerta,
]
