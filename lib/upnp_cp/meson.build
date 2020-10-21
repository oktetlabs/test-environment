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

te_libs += [
    'tools',
    'rpcxdrta',
    'rcfpch',
    'rpcserver',
]
deps += [
    dependency('gobject-2.0'),
    dependency('gssdp-1.0'),
    dependency('gupnp-1.0'),
    dependency('jansson'),
]
