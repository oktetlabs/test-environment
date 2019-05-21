# Copyright (C) 2018-2019 OKTET Labs. All rights reserved.

install_data([ 'daemons/ldap/slapd_run.sh', 'daemons/ldap/slapd.conf' ],
             install_dir: tadir)

includes += include_directories('base')
includes += include_directories('route')
includes += include_directories('rule')
includes += include_directories('util')
includes += include_directories('vm')

sources += files(
    'base/conf.c',
    'base/conf_block.c',
    'base/conf_configfs.c',
    'base/conf_cpu.c',
    'base/conf_eth.c',
    'base/conf_macvlan.c',
    'base/conf_module.c',
    'base/conf_netconsole.c',
    'base/conf_ns_net.c',
    'base/conf_phy.c',
    'base/conf_rlimits.c',
    'base/conf_stats.c',
    'base/conf_sys.c',
    'base/conf_sys_tree.c',
    'base/conf_veth.c',
    'base/conf_vxlan.c',
    'route/conf_route.c',
    'route/conf_route_libnetconf.c',
    'route/conf_route_socket.c',
    'rule/conf_rule.c',
    'util/conf_cmd_monitor.c',
    'util/conf_dlpi.c',
    'util/conf_getmsg.c',
    'util/te_cmd_monitor.c',
    'vm/conf_vm.c',
)

conf = get_option('agent-unix-conf')
if conf.contains('pam')
    dep_pam = cc.find_library('pam')
    if dep_pam.found() and cc.has_function('pam_start', args: te_cflags,
                                           dependencies: dep_pam)
        c_args += [ '-DHAVE_LIBPAM' ]
        deps += [ dep_pam ]
        h = 'security/pam_appl.h'
        if cc.has_header(h)
            c_args += [ '-DHAVE_' + h.to_upper().underscorify() ]
        endif
    else
        warning('No PAM (Pluggable Authentication Modules) support')
    endif
endif
if conf.contains('pci')
    sources += files('base/conf_pci.c')
endif
if conf.contains('vcm')
    sources += files('base/conf_vcm.c')
endif
if conf.contains('tr069')
    sources += files('base/conf_acse.c')
endif
if conf.contains('iptables')
    c_args += [ '-DWITH_IPTABLES' ]
    sources += files('base/conf_iptables.c')
endif
if conf.contains('sniffers')
    dep_pcap = cc.find_library('pcap', args: te_cflags)
    if dep_pcap.found() and cc.has_function('pcap_open_live', args: te_cflags,
                                            dependencies: dep_pcap)
        deps += [ dep_pcap ]
    else
        error('Cannot find pcap library')
    endif
    c_args += [ '-DWITH_SNIFFERS' ]
    sources += files('util/conf_sniffer.c')
    sources += files('util/te_sniffer_proc.c')
endif
if conf.contains('serial-parser')
    c_args += [ '-DWITH_SERIALPARSE' ]
    sources += files('util/conf_serial_parser.c')
endif
if conf.contains('sfptpd')
    c_args += [ '-DWITH_SFPTPD' ]
    sources += files('daemons/sfptpd.c')
endif
if conf.contains('ntpd')
    c_args += [ '-DWITH_NTPD' ]
    sources += files('daemons/ntpd.c')
endif
if conf.contains('8021x')
    c_args += [ '-DENABLE_8021X' ]
    sources += files('daemons/8021x/conf_8021x.c')
endif
if conf.contains('aggr')
    c_args += [ '-DWITH_AGGREGATION' ]
    sources += files('base/conf_aggr.c')
endif
if conf.contains('upnp_cp')
    c_args += [ '-DWITH_UPNP_CP' ]
endif
if conf.contains('tc')
    c_args += [ '-DWITH_TC' ]
    includes += include_directories('tc')
    sources += files(
        'tc/conf_tc.c',
        'tc/conf_qdisc.c',
        'tc/conf_netem.c',
        'tc/conf_tc_internal.c',
        'tc/conf_net_if_wrapper.c',
    )
    # In theory link_whole could be used, but it is indended for
    # internal static libraries build inside the project.
    link_args += [
        '-Wl,-whole-archive',
        '-lnl-route-3',
        '-lnl-3',
        '-Wl,-no-whole-archive',
    ]
    # In theory we need partial dependecy here to add CFLAGS and
    # include directories, but partial_dependency() method is supported
    # since 0.46.0 version only.
    deps += dependency('libnl-3.0')
endif

unix_daemons = get_option('agent-unix-daemons').split()
if unix_daemons.length() != 0
    c_args += [ '-DCFG_UNIX_DAEMONS' ]
    includes += include_directories('daemons')
    sources += files('daemons/conf_daemons.c')
    sources += files('daemons/ldap/ldap.c')
    if unix_daemons.contains('tftp')
        c_args += [ '-DWITH_TFTP_SERVER' ]
        c_args += [ '-DWITH_XINETD' ]
    endif
    if unix_daemons.contains('ftp')
        c_args += [ '-DWITH_FTP_SERVER' ]
        c_args += [ '-DWITH_XINETD' ]
    endif
    if unix_daemons.contains('dns') or unix_daemons.contains('named')
        c_args += [ '-DWITH_DNS_SERVER' ]
        sources += files('daemons/dns/dns_server.c')
        sources += lex_gen.process('daemons/dns/named.lex.l')
        sources += gram_gen.process('daemons/dns/named.conf.y')
    endif
    if unix_daemons.contains('echo')
        c_args += [ '-DWITH_ECHO_SERVER' ]
        c_args += [ '-DWITH_XINETD' ]
    endif
    if unix_daemons.contains('todudp')
        c_args += [ '-DWITH_TODUDP_SERVER' ]
        c_args += [ '-DWITH_XINETD' ]
    endif
    if unix_daemons.contains('telnet')
        c_args += [ '-DWITH_TELNET' ]
        c_args += [ '-DWITH_XINETD' ]
    endif
    if unix_daemons.contains('rsh')
        c_args += [ '-DWITH_RSH' ]
        c_args += [ '-DWITH_XINETD' ]
    endif
    if unix_daemons.contains('dhcp') or unix_daemons.contains('isc-dhcp-server')
        c_args += [ '-DWITH_DHCP_SERVER' ]
        sources += files('daemons/dhcp/dhcp_server.c')
    endif
    if unix_daemons.contains('smtp')
        c_args += [ '-DWITH_SMTP' ]
    endif
    if unix_daemons.contains('vncserver')
        c_args += [ '-DWITH_VNCSERVER' ]
    endif
    if unix_daemons.contains('radvd')
        c_args += [ '-DWITH_RADVD' ]
        sources += files('daemons/radvd/radvd.c')
    endif
    if unix_daemons.contains('openvpn')
        c_args += [ '-DWITH_OPENVPN' ]
        sources += files('daemons/openvpn/openvpn.c')
    endif
    if unix_daemons.contains('pppoe-server')
        c_args += [ '-DWITH_PPPOE_SERVER' ]
        sources += files('daemons/pppoe/pppoe_server.c')
    endif
    if unix_daemons.contains('pppoe-client')
        c_args += [ '-DWITH_PPPOE_CLIENT' ]
        sources += files('daemons/pppoe/pppoe_client.c')
    endif
    if unix_daemons.contains('radius')
        c_args += [ '-DWITH_RADIUS_SERVER' ]
        sources += files('daemons/radius/radius.c')
    endif
    if unix_daemons.contains('vtund')
        c_args += [ '-DWITH_VTUND' ]
        sources += files('daemons/vtund/conf_vtund.c')
    endif
    if unix_daemons.contains('l2tp')
        c_args += [ '-DWITH_L2TP' ]
        sources += files('daemons/l2tp/l2tp_server.c')
    endif
    if unix_daemons.contains('socks')
        c_args += [ '-DWITH_SOCKS' ]
        sources += files('daemons/socks/socks.c')
    endif
endif
