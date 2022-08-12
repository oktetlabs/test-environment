# Copyright (C) 2018-2022 OKTET Labs. All rights reserved.

headers += files(
    'ndn.h',
    'ndn_arp.h',
    'ndn_atm.h',
    'ndn_base.h',
    'ndn_bridge.h',
    'ndn_cli.h',
    'ndn_dhcp.h',
    'ndn_eth.h',
    'ndn_flow.h',
    'ndn_forw.h',
    'ndn_geneve.h',
    'ndn_gre.h',
    'ndn_igmp.h',
    'ndn_internal.h',
    'ndn_ipstack.h',
    'ndn_iscsi.h',
    'ndn_kprobes.h',
    'ndn_llc.h',
    'ndn_pcap.h',
    'ndn_ppp.h',
    'ndn_rte_flow.h',
    'ndn_rte_mbuf.h',
    'ndn_socket.h',
    'ndn_vxlan.h',
)
sources += files(
    'ndn_arp.c',
    'ndn_atm.c',
    'ndn_base.c',
    'ndn_bridge.c',
    'ndn_cli.c',
    'ndn_context.c',
    'ndn_csap.c',
    'ndn_dhcp.c',
    'ndn_eth.c',
    'ndn_flow.c',
    'ndn_forw.c',
    'ndn_generic.c',
    'ndn_geneve.c',
    'ndn_gre.c',
    'ndn_igmp.c',
    'ndn_ipstack.c',
    'ndn_iscsi.c',
    'ndn_kprobes.c',
    'ndn_llc.c',
    'ndn_pcap.c',
    'ndn_ppp.c',
    'ndn_rte_flow.c',
    'ndn_rte_mbuf.c',
    'ndn_socket.c',
    'ndn_vxlan.c',
)

if cc.has_header('net-snmp/definitions.h') or cc.has_header('ucd-snmp/snmp_impl.h')
    headers += files('ndn_snmp.h')
    sources += files('ndn_snmp.c')
    c_args += [ '-DHAS_SNMP', '-DCMU_COMPATIBLE', '-DUCD_COMPATIBLE' ]
endif

te_libs += [ 'asn' ]
