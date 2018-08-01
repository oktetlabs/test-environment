# Copyright (C) 2018 OKTET Labs. All rights reserved.

prj_args = get_option('cflags').split()

c_args += [ '-DALLOW_EXPERIMENTAL_API' ]

# Check if rte_ring_enqueue_bulk() has free space argument
code_ring_enqueue_bulk = '''
#include "rte_config.h"
#include "rte_ring.h"

int main(void) {
    (void)rte_ring_enqueue_bulk(NULL, NULL, 0, NULL);
    return 0;
}
'''
if cc.compiles(code_ring_enqueue_bulk, args: prj_args,
               name: 'rte_ring_enqueue_bulk() free_space argument')
    c_args += [ '-DHAVE_RTE_RING_ENQUEUE_BULK_ARG_FREE_SPACE' ]
endif

# Check if rte_ring_dequeue_bulk() has available argument
code_ring_dequeue_bulk = '''
#include "rte_config.h"
#include "rte_ring.h"

int main(void) {
    (void)rte_ring_dequeue_bulk(NULL, NULL, 0, NULL);
    return 0;
}
'''
if cc.compiles(code_ring_dequeue_bulk, args: prj_args,
               name: 'rte_ring_dequeue_bulk() available argument')
    c_args += [ '-DHAVE_RTE_RING_DEQUEUE_BULK_ARG_AVAILABLE' ]
endif

code_prefix_flow_api_driver = '''
#include "rte_config.h"
#include "rte_flow.h"
#include "rte_flow_driver.h"
'''
if cc.has_member('struct rte_flow_ops', 'isolate', args: prj_args,
                 prefix: code_prefix_flow_api_driver)
    c_args += [ '-DHAVE_STRUCT_RTE_FLOW_OPS_ISOLATE' ]
endif

if cc.has_header_symbol('rte_flow.h', 'RTE_FLOW_ITEM_TYPE_GENEVE',
                        args: prj_args, prefix: '# include "rte_config.h"')
    c_args += [ '-DHAVE_RTE_FLOW_GENEVE' ]
endif

code_prefix_ethdev = '''
#include "rte_config.h"
#include "rte_ethdev.h"
'''
if cc.has_member('struct rte_eth_dev_info', 'rx_queue_offload_capa',
                 args: prj_args, prefix: code_prefix_ethdev)
    c_args += [ '-DHAVE_STRUCT_RTE_ETH_DEV_INFO_RX_QUEUE_OFFLOAD_CAPA' ]
endif
if cc.has_member('struct rte_eth_dev_info', 'tx_queue_offload_capa',
                 args: prj_args, prefix: code_prefix_ethdev)
    c_args += [ '-DHAVE_STRUCT_RTE_ETH_DEV_INFO_TX_QUEUE_OFFLOAD_CAPA' ]
endif
if cc.has_member('struct rte_eth_dev_info', 'dev_capa',
                 args: prj_args, prefix: code_prefix_ethdev)
    c_args += [ '-DHAVE_STRUCT_RTE_ETH_DEV_INFO_DEV_CAPA' ]
endif

code_prefix_flow_api = '''
#include "rte_config.h"
#include "rte_flow.h"
'''
if cc.has_member('struct rte_flow_action_rss', 'rss_conf', args: prj_args,
                 prefix: code_prefix_flow_api)
    c_args += [ '-DHAVE_STRUCT_RTE_FLOW_ITEM_RSS_CONF' ]
endif
if cc.has_member('struct rte_flow_action_rss', 'num', args: prj_args,
                 prefix: code_prefix_flow_api)
    c_args += [ '-DHAVE_STRUCT_RTE_FLOW_ITEM_RSS_NUM' ]
endif
if cc.has_member('struct rte_flow_item_vlan', 'tpid', args: prj_args,
                 prefix: code_prefix_flow_api)
    c_args += [ '-DHAVE_STRUCT_RTE_FLOW_ITEM_VLAN_TPID' ]
endif

sources += files(
    'eal.c',
    'ethdev.c',
    'mbuf.c',
    'mempool.c',
    'ring.c',
    'rte_flow_ndn.c',
    'rte_mbuf_ndn.c',
)
deps += [
    dep_lib_tools,
    dep_lib_rpcserver,
    dep_lib_asn,
    dep_lib_ndn,
    dep_lib_tad,
]
