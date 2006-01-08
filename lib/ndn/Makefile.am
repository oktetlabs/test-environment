
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@

if HAS_SNMP 
AM_CPPFLAGS += -DHAS_SNMP -DCMU_COMPATIBLE -DUCD_COMPATIBLE
SNMP_S = ndn_snmp.c
SNMP_H = ndn_snmp.h
else
SNMP_S =
SNMP_H =
endif


lib_LIBRARIES = libndn.a

include_HEADERS = \
	ndn.h \
	ndn_internal.h \
	ndn_file.h \
	ndn_pcap.h \
	ndn_atm.h \
	ndn_eth.h \
	ndn_bridge.h \
	ndn_arp.h \
	ndn_ipstack.h \
	ndn_forw.h \
	ndn_dhcp.h \
	ndn_cli.h \
	$(SNMP_H) \
	ndn_iscsi.h \
	ndn_socket.h

libndn_a_SOURCES = \
	ndn_generic.c \
	ndn_context.c \
	ndn_file.c \
	ndn_pcap.c \
	ndn_atm.c \
	ndn_eth.c \
	ndn_bridge.c \
	ndn_arp.c \
	ndn_ipstack.c \
	ndn_forw.c \
	ndn_dhcp.c \
	ndn_cli.c \
	$(SNMP_S) \
	ndn_iscsi.c \
	ndn_socket.c


install-exec-hook:
	touch -c -r libndn.a $(DESTDIR)/$(libdir)/libndn.a 

install-data-hook:
	for i in $(include_HEADERS) ; do \
		touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install
