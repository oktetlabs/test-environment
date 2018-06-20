# Copyright (C) 2018 OKTET Labs. All rights reserved.

te_lib_log_bufs = library('log_bufs', 'log_bufs.c', install: install_lib,
                          include_directories: includes)
dep_lib_log_bufs = declare_dependency(link_with: te_lib_log_bufs,
                                     include_directories: includes)

te_lib_logger_file = static_library('logger_file', 'logger_file.c',
                                    install: install_lib,
                                    include_directories: includes)
dep_lib_logger_file = declare_dependency(link_with: te_lib_logger_file,
                                        include_directories: includes)

headers += files(
    'log_bufs.h',
    'logger_file.h',
    'te_alloc.h',
    'te_bufs.h',
    'te_dbuf.h',
    'te_format.h',
    'te_ipstack.h',
    'te_iscsi.h',
    'te_kernel_log.h',
    'te_kvpair.h',
    'te_log_fmt.h',
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
    'tq_string.h',
)
sources += files(
    'te_alloc.c',
    'te_bufs.c',
    'te_dbuf.c',
    'te_format.c',
    'te_ipstack.c',
    'te_kernel_log.c',
    'te_kvpair.c',
    'te_log_file.c',
    'te_log_fmt.c',
    'te_shell_cmd.c',
    'te_sigmap.c',
    'te_sockaddr.c',
    'te_str.c',
    'te_string.c',
    'te_time.c',
    'te_toeplitz.c',
    'te_units.c',
    'tq_string.c',
)
