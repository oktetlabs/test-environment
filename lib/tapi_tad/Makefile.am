
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@ @NET_SNMP_CFLAGS@
AM_LDFLAGS = @TE_LDFLAGS@ 

EXTRA_HEADERS =
EXTRA_SOURCES =
EXTRA_DIST =

# ARP
ARP_HDRS = arp/tapi_arp.h
ARP_SRCS = arp/tapi_arp.c
if WITH_ARP
AM_CPPFLAGS += -I$(srcdir)/arp
EXTRA_HEADERS += $(ARP_HDRS)
EXTRA_SOURCES += $(ARP_SRCS)
else
EXTRA_DIST += $(ARP_HDRS) $(ARP_SRCS)
endif

# ATM
ATM_HDRS = atm/tapi_atm.h
ATM_SRCS = atm/tapi_atm.c
if WITH_ATM
AM_CPPFLAGS += -I$(srcdir)/atm
EXTRA_HEADERS += $(ATM_HDRS)
EXTRA_SOURCES += $(ATM_SRCS)
else
EXTRA_DIST += $(ATM_HDRS) $(ATM_SRCS)
endif

# Bridge/BRIDGE
BRIDGE_HDRS = bridge/tapi_stp.h
BRIDGE_SRCS = bridge/tapi_stp.c
if WITH_BRIDGE
AM_CPPFLAGS += -I$(srcdir)/bridge
EXTRA_HEADERS += $(BRIDGE_HDRS)
EXTRA_SOURCES += $(BRIDGE_SRCS)
else
EXTRA_DIST += $(BRIDGE_HDRS) $(BRIDGE_SRCS)
endif

# CLI
CLI_HDRS = cli/tapi_cli.h
CLI_SRCS = cli/tapi_cli.c
if WITH_CLI
AM_CPPFLAGS += -I$(srcdir)/cli
EXTRA_HEADERS += $(CLI_HDRS)
EXTRA_SOURCES += $(CLI_SRCS)
else
EXTRA_DIST += $(CLI_HDRS) $(CLI_SRCS)
endif

# DHCP
DHCP_HDRS = dhcp/tapi_dhcp.h
DHCP_SRCS = dhcp/tapi_dhcp.c
if WITH_DHCP
AM_CPPFLAGS += -I$(srcdir)/dhcp
EXTRA_HEADERS += $(DHCP_HDRS)
EXTRA_SOURCES += $(DHCP_SRCS)
else
EXTRA_DIST += $(DHCP_HDRS) $(DHCP_SRCS)
endif

# ETH
ETH_HDRS = eth/tapi_eth.h
ETH_SRCS = eth/tapi_eth.c
if WITH_ETH
AM_CPPFLAGS += -I$(srcdir)/eth
EXTRA_HEADERS += $(ETH_HDRS)
EXTRA_SOURCES += $(ETH_SRCS)
else
EXTRA_DIST += $(ETH_HDRS) $(ETH_SRCS)
endif

# IPSTACK
IPSTACK_HDRS = \
	ipstack/tapi_ip4.h \
	ipstack/tapi_ip6.h \
	ipstack/tapi_icmp4.h \
	ipstack/tapi_icmp6.h \
	ipstack/tapi_udp.h \
	ipstack/tapi_tcp.h \
	ipstack/tapi_socket.h
IPSTACK_SRCS = \
	ipstack/tapi_ip4.c \
	ipstack/tapi_ip6.c \
	ipstack/tapi_icmp4.c \
	ipstack/tapi_icmp6.c \
	ipstack/tapi_udp.c \
	ipstack/tapi_tcp.c \
	ipstack/tapi_tcp_conn.c \
	ipstack/tapi_socket.c
if WITH_IPSTACK
AM_CPPFLAGS += -I$(srcdir)/ipstack
EXTRA_HEADERS += $(IPSTACK_HDRS)
EXTRA_SOURCES += $(IPSTACK_SRCS)
else
EXTRA_DIST += $(IPSTACK_HDRS) $(IPSTACK_SRCS)
endif

# IGMP
IGMP_HDRS = igmp/tapi_igmp.h
IGMP_SRCS = igmp/tapi_igmp.c
if WITH_IGMP
AM_CPPFLAGS += -I$(srcdir)/igmp
EXTRA_HEADERS += $(IGMP_HDRS)
EXTRA_SOURCES += $(IGMP_SRCS)
else
EXTRA_DIST += $(IGMP_HDRS) $(IGMP_SRCS)
endif

# PPP
PPP_HDRS = \
            ppp/tapi_ppp.h \
            ppp/tapi_pppoe.h
PPP_SRCS = \
            ppp/tapi_ppp.c \
            ppp/tapi_pppoe.c
if WITH_PPP
AM_CPPFLAGS += -I$(srcdir)/ppp
EXTRA_HEADERS += $(PPP_HDRS)
EXTRA_SOURCES += $(PPP_SRCS)
else
EXTRA_DIST += $(PPP_HDRS) $(PPP_SRCS)
endif

# ISCSI
ISCSI_HDRS = iscsi/tapi_iscsi.h
ISCSI_SRCS = iscsi/tapi_iscsi.c
if WITH_ISCSI
AM_CPPFLAGS += -I$(srcdir)/iscsi
EXTRA_HEADERS += $(ISCSI_HDRS)
EXTRA_SOURCES += $(ISCSI_SRCS)
else
EXTRA_DIST += $(ISCSI_HDRS) $(ISCSI_SRCS)
endif

# PCAP
PCAP_HDRS = pcap/tapi_pcap.h
PCAP_SRCS = pcap/tapi_pcap.c
if WITH_PCAP
AM_CPPFLAGS += -I$(srcdir)/pcap
EXTRA_HEADERS += $(PCAP_HDRS)
EXTRA_SOURCES += $(PCAP_SRCS)
else
EXTRA_DIST += $(PCAP_HDRS) $(PCAP_SRCS)
endif

# FLOW
FLOW_HDRS = flow/tapi_flow.h
FLOW_SRCS = flow/tapi_flow.c
if WITH_FLOW
AM_CPPFLAGS += -I$(srcdir)/flow
EXTRA_HEADERS += $(FLOW_HDRS)
EXTRA_SOURCES += $(FLOW_SRCS)
else
EXTRA_DIST += $(FLOW_HDRS) $(FLOW_SRCS)
endif

# SNMP
SNMP_HDRS = \
	snmp/tapi_snmp.h \
	snmp/tapi_snmp_defs.h \
	snmp/tapi_snmp_iftable.h \
	snmp/tapi_snmp_ip_addrtable.h \
	snmp/tapi_snmp_ip_mibs.h \
	snmp/tapi_snmp_macro.h
SNMP_SRCS = snmp/tapi_snmp.c
if WITH_SNMP
AM_CPPFLAGS += -I$(srcdir)/snmp
EXTRA_HEADERS += $(SNMP_HDRS)
EXTRA_SOURCES += $(SNMP_SRCS)
else
EXTRA_DIST += $(SNMP_HDRS) $(SNMP_SRCS)
endif

# FORW
FORW_HDRS = forw/tapi_forw.h
FORW_SRCS = forw/tapi_forw.c
if WITH_FORW
AM_CPPFLAGS += -I$(srcdir)/forw
EXTRA_HEADERS += $(FORW_HDRS)
EXTRA_SOURCES += $(FORW_SRCS)
else
EXTRA_DIST += $(FORW_HDRS) $(FORW_SRCS)
endif

include_HEADERS = \
	tapi_tad.h \
	tapi_ndn.h \
	$(EXTRA_HEADERS)

lib_LTLIBRARIES = libtapi_tad.la

libtapi_tad_la_SOURCES = \
	tapi_tad.c \
	tapi_ndn.c \
	$(EXTRA_SOURCES)


install-exec-hook:
	touch -c -r libtapi_tad.la $(DESTDIR)/$(libdir)/libtapi_tad.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
		touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$(basename $$i) ; \
	done

all: install
