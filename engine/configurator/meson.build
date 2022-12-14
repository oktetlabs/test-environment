# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2018-2022 OKTET Labs Ltd. All rights reserved.

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
    'conf_rcf.c',
    'conf_ta.c',
    'conf_print.c'
]

te_cs_deps = [
    dep_libxml2,
    dep_popt,
    dep_threads,

    dep_lib_rcfapi,
    dep_lib_ipcserver,
    dep_lib_conf_types,
    dep_lib_conf_oid,
    dep_lib_confapi,
    dep_lib_static_conf_ipc,
    dep_lib_logger_ten,
    dep_lib_ipc,
    dep_lib_logic_expr,
    dep_lib_tools,
    dep_lib_logger_core,
]

if get_option('cs-conf-yaml')
    dep_yaml = dependency('yaml-0.1', required: false)
    required_deps += 'yaml-0.1'
    if dep_yaml.found()
        te_cs_deps += dep_yaml
        c_args += [ '-DWITH_CONF_YAML' ]
        sources += [ 'conf_yaml.c' ]
    else
        missed_deps += 'yaml-0.1'
    endif
endif

executable('te_cs', sources, install: true,
           c_args: c_args,
           dependencies: te_cs_deps)

executable('te_cs_shutdown', 'conf_shutdown.c', install: true,
           c_args: c_args,
           dependencies: [ dep_lib_confapi, dep_lib_logger_ten,
                           dep_lib_ipc, dep_lib_tools, dep_lib_logger_core ])

install_data(
    'subtree_backup.xsl',
    install_dir: join_paths(get_option('datadir'), 'xsl'),
)