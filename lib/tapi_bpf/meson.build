# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2019-2022 OKTET Labs Ltd. All rights reserved.

headers += files(
    'tapi_bpf.h',
    'tapi_bpf_rxq_stats.h',
    'tapi_bpf_stim.h',
)
sources += files(
    'tapi_bpf.c',
    'tapi_bpf_rxq_stats.c',
    'tapi_bpf_stim.c',
)
te_libs += [
    'rpcxdr',
    'tapi',
    'tapi_rpc',
]

if get_option('rpcdefs').split(',').contains('tarpc_bpf.x.m4')
    headers += files('tapi_rpc_bpf.h')
    sources += files('tapi_rpc_bpf.c')
endif
