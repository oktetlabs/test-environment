# Copyright (C) 2018-2019 OKTET Labs. All rights reserved.

rcf_sniffers_dummy = library('rcf_sniffers_dummy',
                             'rcf_sniffers_dummy.c', install: install_lib,
                             dependencies: dep_lib_conf_oid)
dep_lib_rcf_sniffers_dummy = declare_dependency(link_with: rcf_sniffers_dummy,
                                                dependencies: dep_lib_conf_oid)

if get_option('libs').split().contains('netconf')
    c_args += [ '-DWITH_NETCONF' ]
    deps += [ dep_lib_static_netconf ]
endif

f = 'pthread_atfork'
if dep_threads.found() and cc.has_function(f, dependencies: dep_threads)
    c_args += [ '-DHAVE_' + f.to_upper() ]
endif

headers += files(
    'rcf_ch_api.h',
    'rcf_pch.h',
    'rcf_pch_ta_cfg.h',
)
sources += files(
    'rcf_pch.c',
    'rcf_pch_conf.c',
    'rcf_pch_file.c',
    'rcf_pch_plugin.c',
    'rcf_pch_rpc.c',
    'rcf_pch_ta_cfg.c',
    'rcf_pch_var.c',
)
te_libs += [
    'tools',
    'conf_oid',
    'loggerta',
    'rpcxdr',
    'rpctransport',
    'agentlib',
]
