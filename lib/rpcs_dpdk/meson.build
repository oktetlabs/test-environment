# Copyright (C) 2018-2019 OKTET Labs. All rights reserved.

c_args += [ '-DALLOW_EXPERIMENTAL_API' ]

# Check if specified rte_mbuf fields present
code_prefix_mbuf = '''
#include "rte_config.h"
#include "rte_mbuf.h"
'''
mbuf_fields = [
    'seqn',
]
foreach f : mbuf_fields
    if cc.has_member('struct rte_mbuf', f, args: te_cflags,
                     prefix: code_prefix_mbuf)
        c_args += [ '-DHAVE_STRUCT_RTE_MBUF_' + f.to_upper().underscorify() ]
    endif
endforeach

# Check if rte_ring_enqueue_bulk() has free space argument
code_ring_enqueue_bulk = '''
#include "rte_config.h"
#include "rte_ring.h"

int main(void) {
    (void)rte_ring_enqueue_bulk(NULL, NULL, 0, NULL);
    return 0;
}
'''
if cc.compiles(code_ring_enqueue_bulk, args: te_cflags,
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
if cc.compiles(code_ring_dequeue_bulk, args: te_cflags,
               name: 'rte_ring_dequeue_bulk() available argument')
    c_args += [ '-DHAVE_RTE_RING_DEQUEUE_BULK_ARG_AVAILABLE' ]
endif

# Check if rte_eth_promiscuous_enable() returns value
code_promiscuous = '''
#include <stdint.h>
#include "rte_ethdev.h"

int main(void) {
    typedef void (*func)(uint16_t);
    func f = rte_eth_promiscuous_enable;
    (void)f;
    return 0;
}
'''
if cc.compiles(code_promiscuous, args: te_cflags + ['-Werror'],
               name: 'rte_eth_promiscuous_enable() returns value ')
    c_args += [ '-DHAVE_RTE_PROMISCUOUS_RETURN_VOID' ]
endif

# Check if rte_eth_allmulticast_enable() returns value
code_allmulticast = '''
#include <stdint.h>
#include "rte_ethdev.h"

int main(void) {
    typedef void (*func)(uint16_t);
    func f = rte_eth_allmulticast_enable;
    (void)f;
    return 0;
}
'''
if cc.compiles(code_promiscuous, args: te_cflags + ['-Werror'],
               name: 'rte_eth_allmulticast_enable() returns value ')
    c_args += [ '-DHAVE_RTE_ALLMULTICAST_RETURN_VOID' ]
endif

# Check if rte_eth_allmulticast_enable() returns value
code_dev_info_return= '''
#include <stdint.h>
#include "rte_ethdev.h"

int main(void) {
    typedef void (*func)(uint16_t, struct rte_eth_dev_info *);
    func f = rte_eth_dev_info_get;
    (void)f;
    return 0;
}
'''
if cc.compiles(code_dev_info_return, args: te_cflags + ['-Werror'],
               name: 'rte_eth_dev_info_get() returns value ')
    c_args += [ '-DHAVE_RTE_DEV_INFO_GET_RETURN_VOID' ]
endif

t = 'struct rte_ether_addr'
if cc.has_type(t, args: te_cflags,
               prefix: '#include "rte_ethdev.h"')
    c_args += [ '-DHAVE_' + t.to_upper().underscorify() ]
endif

if cc.has_header_symbol('rte_flow.h', 'RTE_FLOW_ITEM_TYPE_GENEVE',
                        args: te_cflags, prefix: '# include "rte_config.h"')
    c_args += [ '-DHAVE_RTE_FLOW_GENEVE' ]
endif

code_prefix_ethdev = '''
#include "rte_config.h"
#include "rte_ethdev.h"
'''
if cc.has_member('struct rte_eth_dev_info', 'rx_queue_offload_capa',
                 args: te_cflags, prefix: code_prefix_ethdev)
    c_args += [ '-DHAVE_STRUCT_RTE_ETH_DEV_INFO_RX_QUEUE_OFFLOAD_CAPA' ]
endif
if cc.has_member('struct rte_eth_dev_info', 'tx_queue_offload_capa',
                 args: te_cflags, prefix: code_prefix_ethdev)
    c_args += [ '-DHAVE_STRUCT_RTE_ETH_DEV_INFO_TX_QUEUE_OFFLOAD_CAPA' ]
endif
if cc.has_member('struct rte_eth_dev_info', 'dev_capa',
                 args: te_cflags, prefix: code_prefix_ethdev)
    c_args += [ '-DHAVE_STRUCT_RTE_ETH_DEV_INFO_DEV_CAPA' ]
endif
t = 'struct rte_eth_dev_portconf'
if cc.has_type(t, prefix: code_prefix_ethdev)
    c_args += [ '-DHAVE_' + t.to_upper().underscorify() ]
endif

code_prefix_flow_api = '''
#include "rte_config.h"
#include "rte_flow.h"
'''
if cc.has_member('struct rte_flow_action_rss', 'rss_conf', args: te_cflags,
                 prefix: code_prefix_flow_api)
    c_args += [ '-DHAVE_STRUCT_RTE_FLOW_ACTION_RSS_RSS_CONF' ]
endif
if cc.has_member('struct rte_flow_action_rss', 'num', args: te_cflags,
                 prefix: code_prefix_flow_api)
    c_args += [ '-DHAVE_STRUCT_RTE_FLOW_ACTION_RSS_NUM' ]
endif
if cc.has_member('struct rte_flow_item_vlan', 'tpid', args: te_cflags,
                 prefix: code_prefix_flow_api)
    c_args += [ '-DHAVE_STRUCT_RTE_FLOW_ITEM_VLAN_TPID' ]
endif

f = 'rte_pktmbuf_pool_create_by_ops'
if cc.has_function(f, args: te_cflags)
    c_args += [ '-DHAVE_' + f.to_upper() ]
endif

f = 'rte_is_zero_ether_addr'
if cc.has_function(f, args: te_cflags,
                   prefix: '#include "rte_ether.h"')
    c_args += [ '-DHAVE_' + f.to_upper() ]
endif

f = 'rte_flow_isolate'
if cc.has_function(f, args: te_cflags + te_ldflags +
                   [ '-lrte_ethdev', '-lrte_telemetry', '-lrte_meter',
                     '-lrte_net', '-lrte_mbuf', '-lrte_mempool',
                     '-lrte_ring', '-lrte_kvargs', '-lrte_eal' ],
                   prefix: '#include "rte_flow.h"')
    c_args += [ '-DHAVE_' + f.to_upper() ]
endif

sources += files(
    'dpdk.c',
    'eal.c',
    'ethdev.c',
    'mbuf.c',
    'mempool.c',
    'ring.c',
    'rte_flow_ndn.c',
    'rte_mbuf_ndn.c',
)
te_libs += [
    'tools',
    'rpcserver',
    'asn',
    'ndn',
    'tad',
    'rpc_dpdk',
]
