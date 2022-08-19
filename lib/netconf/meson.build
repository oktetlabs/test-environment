# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2018-2022 OKTET Labs Ltd. All rights reserved.

conf_data = configuration_data()

# Check if Geneve is supported
code_geneve = '''
#include <linux/if_link.h>

int main(void) {
    return IFLA_GENEVE_ID;
}
'''
if cc.compiles(code_geneve, args: te_cflags,
               name: 'IFLA_GENEVE_ID available')
    c_args += ['-DHAVE_DECL_IFLA_GENEVE_ID']
endif

# Check if IPv6 address type VXLAN attributes supported
code_vxlan_local6 = '''
#include <linux/if_link.h>

int main(void) {
    return IFLA_VXLAN_LOCAL6;
}
'''
if cc.compiles(code_vxlan_local6, args: te_cflags,
               name: 'IFLA_VXLAN_LOCAL6 available')
    c_args += ['-DHAVE_DECL_IFLA_VXLAN_LOCAL6']
endif

code_vxlan_group6 = '''
#include <linux/if_link.h>

int main(void) {
    return IFLA_VXLAN_GROUP6;
}
'''
if cc.compiles(code_vxlan_group6, args: te_cflags,
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
if cc.compiles(code_geneve_remote6, args: te_cflags,
               name: 'IFLA_GENEVE_REMOTE6 available')
    c_args += ['-DHAVE_DECL_IFLA_GENEVE_REMOTE6']
endif

if cc.has_header('linux/genetlink.h')
    conf_data.set('HAVE_LINUX_GENETLINK_H', 1)
endif

if cc.has_header('linux/devlink.h')
    conf_data.set('HAVE_LINUX_DEVLINK_H', 1)

    devlink_cmds = [
        'DEVLINK_CMD_INFO_GET',
        'DEVLINK_CMD_PARAM_GET',
        'DEVLINK_CMD_PARAM_SET',
    ]

    foreach d : devlink_cmds
        if cc.has_header_symbol('linux/devlink.h', d)
            conf_data.set('HAVE_DECL_' + d, 1)
        else
            conf_data.set('HAVE_DECL_' + d, 0)
        endif
    endforeach
endif

configure_file(output: 'config.h',
               configuration: conf_data)

headers += files(
    'netconf.h',
    'netconf_internal.h',
    'netconf_internal_genetlink.h',
)
sources += files(
    'bridge.c',
    'devlink.c',
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
    'genetlink.c',
    'geneve.c',
    'vxlan.c',
)
te_libs += [
    'tools',
    'conf_oid',
]
