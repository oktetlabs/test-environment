# Copyright (C) 2019 OKTET Labs. All rights reserved.

headers += files(
    'tapi_dpdk.h',
    'tapi_dpdk_stats.h',
    'tapi_rte_flow.h',
)
sources += files(
    'tapi_dpdk.c',
    'tapi_dpdk_stats.c',
    'tapi_rte_flow.c',
)
deps += [
    dep_lib_rpcc_dpdk,
    dep_lib_tapi,
    dep_lib_tapi_job,
]
