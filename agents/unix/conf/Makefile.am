
ACLOCAL_AMFLAGS = -I $(TE_M4)

AM_CPPFLAGS = -I$(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/include $(TE_CPPFLAGS) \
              -I$(srcdir)/.. -I$(srcdir)/base -I$(srcdir)/util -I$(srcdir)/vm -I$(srcdir)/process \
              -I$(srcdir)/ovs -I$(srcdir)/route -I$(srcdir)/rule -I$(srcdir)/daemons -I. \
              @EXTRA_CPPFLAGS@ $(TE_CPPFLAGS_VAR)

AM_CFLAGS = -fno-strict-aliasing $(TE_CFLAGS) $(TE_CFLAGS_VAR)

AM_YFLAGS = -d

tadir=$(prefix)/@with_name@

ta_DATA=daemons/ldap/slapd.conf

ta_SCRIPTS=daemons/ldap/slapd_run.sh

EXTRA_DIST=daemons/ldap/slapd.conf daemons/ldap/slapd_run.sh

noinst_HEADERS = \
	base/conf_netconf.h \
	base/conf_common.h \
	base/conf_ethtool.h \
	ovs/conf_ovs.h \
	process/conf_process.h \
	route/conf_route.h \
	rule/conf_rule.h \
	util/conf_dlpi.h \
	util/te_cmd_monitor.h \
	util/conf_getmsg.h \
	vm/conf_vm.h

noinst_LIBRARIES=libconf.a

libconf_a_SOURCES = \
	base/conf.c \
	base/conf_eth.c \
	base/conf_iommu.c \
	base/conf_stats.c \
	base/conf_coalesce.c \
	base/conf_sys.c \
	base/conf_sys_tree.c \
	base/conf_phy.c \
	base/conf_netconsole.c \
	base/conf_configfs.c \
	base/conf_cpu.c \
	base/conf_ipvlan.c \
	base/conf_l4_port.c \
	base/conf_macvlan.c \
	base/conf_module.c \
	base/conf_ns_net.c \
	base/conf_veth.c \
	base/conf_udp_tunnel.c \
	base/conf_bridge.c \
	base/conf_rlimits.c \
	base/conf_block.c \
	base/conf_ethtool.c \
	process/conf_process.c \
	route/conf_route.c \
	route/conf_route_libnetconf.c \
	route/conf_route_socket.c \
	rule/conf_rule.c \
	util/conf_getmsg.c \
	util/conf_dlpi.c \
	util/te_cmd_monitor.c \
	util/conf_cmd_monitor.c \
	vm/conf_vm.c

CLEANFILES=

if ISCSI_TARGET_USE

AM_CPPFLAGS += -I$(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/include/iscsi_unh_target
libconf_a_SOURCES += conf_iscsi_target.c

endif

if CFG_PCI
libconf_a_SOURCES += base/conf_pci.c
endif

if CFG_VCM
libconf_a_SOURCES += base/conf_vcm.c
endif

if CFG_TR069
libconf_a_SOURCES += base/conf_acse.c
endif

if CFG_IPTABLES_SUPPORT
libconf_a_SOURCES += base/conf_iptables.c
endif

if CFG_BPF_SUPPORT
libconf_a_SOURCES += util/conf_bpf.c
endif

if CFG_SNIFFERS_SUPPORT
libconf_a_SOURCES += util/conf_sniffer.c
libconf_a_SOURCES += util/te_sniffer_proc.c
endif

if CFG_SERIALPARSE_SUPPORT
libconf_a_SOURCES += util/conf_serial_parser.c
endif

if CFG_SERIAL_SUPPORT
libconf_a_SOURCES += util/conf_serial.c
endif

if CFG_SFPTPD_SUPPORT
libconf_a_SOURCES += daemons/sfptpd.c
endif

if CFG_NTPD_SUPPORT
libconf_a_SOURCES += daemons/ntpd.c
endif

if CFG_UNIX_8021X
libconf_a_SOURCES += daemons/8021x/conf_8021x.c
endif

if CFG_UNIX_DAEMONS

AM_CPPFLAGS += @DAEMONS_CFLAGS@

libconf_a_SOURCES += daemons/conf_daemons.c daemons/ldap/ldap.c
noinst_HEADERS += daemons/conf_daemons.h daemons/conf_daemons_internal.h

if CFG_UNIX_DAEMONS_RADVD

libconf_a_SOURCES += daemons/radvd/radvd.c
noinst_HEADERS += daemons/radvd/radvd.h

endif

if CFG_UNIX_DAEMONS_ISC_DHCP_SERVER

libconf_a_SOURCES += daemons/dhcp/dhcp_server.c
noinst_HEADERS += daemons/dhcp/dhcp_server.h

endif

if CFG_UNIX_DAEMONS_OPENVPN

libconf_a_SOURCES += daemons/openvpn/openvpn.c

endif

if CFG_UNIX_DAEMONS_PPPOE_SERVER
libconf_a_SOURCES += daemons/pppoe/pppoe_server.c
endif

if CFG_UNIX_DAEMONS_PPPOE_CLIENT
libconf_a_SOURCES += daemons/pppoe/pppoe_client.c
endif

if CFG_UNIX_DAEMONS_NAMED

libconf_a_SOURCES += daemons/dns/dns_server.c \
                     daemons/dns/named.conf.y \
                     daemons/dns/named.lex.l

CLEANFILES+=named.conf.c named.lex.c named.conf.h

endif

if CFG_UNIX_DAEMONS_RADIUS
libconf_a_SOURCES += daemons/radius/radius.c
endif

if CFG_UNIX_DAEMONS_VTUND
libconf_a_SOURCES += daemons/vtund/conf_vtund.c
endif

if CFG_UNIX_DAEMONS_L2TP
libconf_a_SOURCES += daemons/l2tp/l2tp_server.c
endif

if CFG_UNIX_DAEMONS_SOCKS
libconf_a_SOURCES += daemons/socks/socks.c
endif

if CFG_UNIX_DAEMONS_NGINX
libconf_a_SOURCES += \
	daemons/nginx/conf_nginx.c \
	daemons/nginx/conf_nginx_http.c
noinst_HEADERS += daemons/nginx/conf_nginx.h
endif

endif


if CFG_UNIX_AGGR
libconf_a_SOURCES += base/conf_aggr.c
endif

if ISCSI_TARGET_USE
libconf_a_DEPENDENCIES = \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libiscsi_initiator_conf.a
endif

if CFG_UNIX_OVS
libconf_a_SOURCES += ovs/conf_ovs.c
endif

if CFG_UNIX_TC
libconf_a_SOURCES += tc/conf_tc.c
libconf_a_SOURCES += tc/conf_qdisc.c
libconf_a_SOURCES += tc/conf_qdisc_params.c
libconf_a_SOURCES += tc/conf_tc_internal.c
libconf_a_SOURCES += tc/conf_net_if_wrapper.c
noinst_HEADERS += tc/conf_tc.h
noinst_HEADERS += tc/conf_qdisc.h
noinst_HEADERS += tc/conf_qdisc_params.h
noinst_HEADERS += tc/conf_tc_internal.h
libconf_a_SOURCES += tc/conf_net_if_wrapper.h
endif
