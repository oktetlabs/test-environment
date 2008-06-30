
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@ 
AM_LDFLAGS = @TE_LDFLAGS@ 

include_HEADERS = \
	tapi_test.h \
	tapi_test_log.h \
	tapi_jmp.h \
	tapi_sockaddr.h \
	tapi_cfg.h \
	tapi_cfg_base.h \
	tapi_cfg_phy.h \
	tapi_cfg_net.h \
	tapi_cfg_arl.h \
	tapi_cfg_dhcp.h \
	tapi_cfg_pppoe.h \
	tapi_cfg_vtund.h \
	tapi_cfg_ip6.h \
	tapi_cfg_stats.h \
	tapi_cfg_xen.h \
	tapi_file.h \
	tapi_reqs.h

lib_LTLIBRARIES = libtapi.la

libtapi_la_SOURCES = \
	test_params.c \
	tapi_jmp.c \
	tapi_sockaddr.c \
	tapi_cfg.c \
	tapi_cfg_local_to_ta.c \
	tapi_cfg_base.c \
	tapi_cfg_phy.c \
	tapi_cfg_net.c \
	tapi_cfg_arl.c \
	tapi_cfg_dhcp.c \
	tapi_cfg_pppoe.c \
	tapi_cfg_vtund.c \
	tapi_cfg_ip6.c \
	tapi_cfg_stats.c \
	tapi_cfg_xen.c \
	tapi_file.c \
	verdict.c \
	tapi_reqs.c

install-exec-hook:
	touch -c -r libtapi.la $(DESTDIR)/$(libdir)/libtapi.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
		touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install
