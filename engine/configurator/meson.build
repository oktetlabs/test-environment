# Copyright (C) 2018 OKTET Labs. All rights reserved.

c_args = []

if cc.has_type('xmlError', prefix: '#include <libxml/xmlerror.h>',
               dependencies: dep_libxml2)
    c_args += [ '-DHAVE_XMLERROR' ]
endif

sources = [
    'conf_db.c',
    'conf_dh.c',
    'conf_main.c',
    'conf_backup.c',
    'conf_ta.c',
    'conf_print.c'
]

executable('te_cs', sources, install: true,
           c_args: c_args,
           dependencies: [ dep_libxml2, dep_popt, dep_threads,
                           dep_lib_rcfapi, dep_lib_ipcserver,
                           dep_lib_conf_types, dep_lib_conf_oid,
                           dep_lib_confapi, dep_lib_logger_ten,
                           dep_lib_ipc, dep_lib_tools ])

executable('te_cs_shutdown', 'conf_shutdown.c', install: true,
           c_args: c_args,
           dependencies: [ dep_lib_confapi, dep_lib_logger_ten,
                           dep_lib_ipc, dep_lib_tools ])
