# Copyright (C) 2018-2019 OKTET Labs. All rights reserved.

prj_args = get_option('cflags').split()

# Check if IPv6 address type VXLAN attributes supported
code_vxlan_local6 = '''
#include <linux/if_link.h>

int main(void) {
    return IFLA_VXLAN_LOCAL6;
}
'''
if cc.compiles(code_vxlan_local6, args: prj_args,
               name: 'IFLA_VXLAN_LOCAL6 available')
    c_args += ['-DHAVE_DECL_IFLA_VXLAN_LOCAL6']
endif

code_vxlan_group6 = '''
#include <linux/if_link.h>

int main(void) {
    return IFLA_VXLAN_GROUP6;
}
'''
if cc.compiles(code_vxlan_group6, args: prj_args,
               name: 'IFLA_VXLAN_GROUP6 available')
    c_args += ['-DHAVE_DECL_IFLA_VXLAN_GROUP6']
endif

# Check if IPv6 address type Geneve attributes supported
code_geneve_remote6 = '''
#include <linux/if_link.h>

int main(void) {
    return IFLA_GENEVE_REMOTE6;
}
'''
if cc.compiles(code_geneve_remote6, args: prj_args,
               name: 'IFLA_GENEVE_REMOTE6 available')
    c_args += ['-DHAVE_DECL_IFLA_GENEVE_REMOTE6']
endif

headers += files(
    'netconf.h',
    'netconf_internal.h',
)
sources += files(
    'bridge.c',
    'ipvlan.c',
    'link.c',
    'macvlan.c',
    'neigh.c',
    'net_addr.c',
    'netconf.c',
    'route.c',
    'rule.c',
    'veth.c',
    'vlan.c',
    'udp_tunnel.c',
    'geneve.c',
    'vxlan.c',
)
deps += [
    dep_lib_conf_oid,
]
