
ACLOCAL_AMFLAGS = -I @TE_M4@

include_HEADERS = \
	te_rpc_types.h \
	te_rpc_defs.h \
	te_rpc_aio.h \
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


install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install
