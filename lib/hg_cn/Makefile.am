
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@

lib_LTLIBRARIES = libhg_cn.la

include_HEADERS = \
	hg_cn.h \
	hg_cn_lan.h \
	hg_cn_wan.h \
	hg_cn_wan_layer.h

libhg_cn_la_SOURCES = \
	hg_cn.c \
	lan.c \
	wan.c \
	wan_atm.c \
	wan_eth.c \
	wan_layer_bridge.c \
	wan_layer_eth_if.c \
	wan_layer_firewall.c \
	wan_layer_ip_dhcp.c \
	wan_layer_ip_ipcp.c \
	wan_layer_ip_static.c \
	wan_layer_ppp.c


install-exec-hook:
	touch -c -r libhg_cn.la $(DESTDIR)/$(libdir)/libhg_cn.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
		touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install
