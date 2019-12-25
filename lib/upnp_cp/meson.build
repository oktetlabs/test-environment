# Copyright (C) 2019 OKTET Labs. All rights reserved.

headers += files(
    'conf_upnp_cp.h',
    'te_upnp_cp.h',
)

sources += files(
    'conf_upnp_cp.c',
    'tarpc_upnp_cp.c',
    'te_upnp_cp.c',
)

deps += [
    dep_lib_tools,
    dep_lib_rpcxdrta,
    dep_lib_rcfpch,
    dep_lib_rpcserver,
    dependency('gupnp-1.0'),
]
