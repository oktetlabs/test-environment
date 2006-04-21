
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@


include_HEADERS = \
	te_rpc_types.h \
	te_rpc_defs.h \
	te_rpc_aio.h \
	te_rpc_errno.h \
	te_rpc_fcntl.h \
	te_rpc_netdb.h \
	te_rpc_net_if_arp.h \
	te_rpc_net_if.h \
	te_rpc_signal.h \
	te_rpc_sys_poll.h \
	te_rpc_sys_resource.h \
	te_rpc_sys_time.h \
	te_rpc_sys_socket.h \
	te_rpc_sys_stat.h \
	te_rpc_sys_wait.h \
	te_rpc_wsa.h


lib_LTLIBRARIES=librpc_types.la

librpc_types_la_SOURCES = \
	defs.c \
	aio.c \
	fcntl.c \
	netdb.c \
	net_if_arp.c \
	net_if.c \
	signal.c \
	sys_poll.c \
	sys_resource.c \
	sys_socket.c \
	sys_stat.c \
	sys_wait.c \
	wsa.c


install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

install-exec-hook:
	touch -c -r librpc_types.* $(DESTDIR)/$(libdir)/librpc_types.*

all: install
