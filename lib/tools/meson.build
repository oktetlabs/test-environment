# Copyright (C) 2018 OKTET Labs. All rights reserved.

te_lib_logger_file = static_library('logger_file', 'te_log_file.c',
                                    install: install_lib,
                                    include_directories: includes,
                                    dependencies: dep_lib_logger_core)
dep_lib_logger_file = declare_dependency(link_with: te_lib_logger_file,
                                        include_directories: includes)
dep_lib_static_logger_file = dep_lib_logger_file

headers += files(
    'log_bufs.h',
    'logger_file.h',
    'te_alloc.h',
    'te_bufs.h',
    'te_dbuf.h',
    'te_exec_child.h',
    'te_expand.h',
    'te_file.h',
    'te_format.h',
    'te_hex_diff_dump.h',
    'te_ipstack.h',
    'te_iscsi.h',
    'te_kernel_log.h',
    'te_kvpair.h',
    'te_log_fmt.h',
    'te_log_stack.h',
    'te_meas_stats.h',
    'te_mi_log.h',
    'te_serial.h',
    'te_serial_common.h',
    'te_serial_parser.h',
    'te_shell_cmd.h',
    'te_sigmap.h',
    'te_sleep.h',
    'te_sockaddr.h',
    'te_str.h',
    'te_string.h',
    'te_time.h',
    'te_toeplitz.h',
    'te_tools.h',
    'te_units.h',
    'te_vector.h',
    'te_yaml.h',
    'tq_string.h',
)
sources += files(
    'log_bufs.c',
    'te_alloc.c',
    'te_bufs.c',
    'te_dbuf.c',
    'te_exec_child.c',
    'te_expand.c',
    'te_file.c',
    'te_format.c',
    'te_hex_diff_dump.c',
    'te_ipstack.c',
    'te_kernel_log.c',
    'te_kvpair.c',
    'te_log_fmt.c',
    'te_log_stack.c',
    'te_meas_stats.c',
    'te_mi_log.c',
    'te_shell_cmd.c',
    'te_sigmap.c',
    'te_sockaddr.c',
    'te_str.c',
    'te_string.c',
    'te_time.c',
    'te_toeplitz.c',
    'te_units.c',
    'te_vector.c',
    'tq_string.c',
)

#
# Check for BSD string header and functions
#
bsd_funcs_prefix = ''
h = 'bsd/string.h'
if cc.has_header(h, args: c_args)
    c_args += [ '-DHAVE_' + h.to_upper().underscorify() ]
    bsd_funcs_prefix = '#include <bsd/string.h>'
endif

dep_bsd = dependency('libbsd', required: false)
bsd_funcs = [
    'strlcpy',
    'strlcat',
]
foreach f : bsd_funcs
    if cc.has_function(f, args: c_args, prefix: bsd_funcs_prefix)
        c_args += [ '-DHAVE_' + f.to_upper() ]
    else
        if dep_bsd.found() and cc.has_function(f, args: c_args,
                                               prefix: bsd_funcs_prefix,
                                               dependencies: dep_bsd)
            c_args += [ '-DHAVE_' + f.to_upper() ]
            deps += [ dep_bsd ]
        endif
    endif
endforeach

if cc.has_function('getaddrinfo')
    c_args += [ '-DHAVE_GETADDRINFO' ]
endif

deps += [ cc.find_library('m'), dep_threads ]

h = 'sched.h'
if cc.has_header(h)
    c_args += [ '-DHAVE_' + h.to_upper().underscorify() ]
endif

sched_funcs = [
    'sched_setaffinity',
    'sched_getaffinity',
]

foreach f : sched_funcs
    if cc.has_function(f)
        c_args += [ '-DHAVE_' + f.to_upper() ]
    endif
endforeach
