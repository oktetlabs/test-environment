# Copyright (C) 2018 OKTET Labs. All rights reserved.

headers += files(
    'rte_flow_ndn.h',
    'tapi_rpc_dpdk.h',
    'tapi_rpc_rte.h',
    'tapi_rpc_rte_eal.h',
    'tapi_rpc_rte_ethdev.h',
    'tapi_rpc_rte_flow.h',
    'tapi_rpc_rte_mbuf.h',
    'tapi_rpc_rte_mbuf_ndn.h',
    'tapi_rpc_rte_mempool.h',
    'tapi_rpc_rte_ring.h',
    'tapi_rte_mbuf.h',
)
sources += files(
    'dpdk.c',
    'eal.c',
    'ethdev.c',
    'mbuf.c',
    'mempool.c',
    'ring.c',
    'rte_flow_ndn.c',
    'tapi_ethdev.c',
    'tapi_rpc_rte_mbuf_ndn.c',
    'tapi_rte_mbuf.c',
)
deps += [
    dep_lib_tools,
    dep_lib_log_bufs,
    dep_lib_rpcxdr,
    dep_lib_tapi,
    dep_lib_tapi_env,
    dep_lib_tapi_tad,
    dep_lib_rpc_dpdk,
]
