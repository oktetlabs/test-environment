
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

if WINDOWS

WIN_SRC=defs.c wsa.c sys_socket.c fcntl.c

lib_LIBRARIES=librpc_types.a

librpc_types_a_CFLAGS=-DTE_DUMMY $(AM_CFLAGS)
librpc_types_a_SOURCES=$(WIN_SRC)

librpc_types.a: $(WIN_SRC) 
	TE_INSTALL=$(DESTDIR)/$(prefix)/.. \
        $(srcdir)/../../engine/builder/te_build_win_lib $(srcdir) \
        $(TE_PLATFORM) $@ $(DESTDIR)/$(prefix) $(WIN_SRC)

install-exec-hook:
	if test -e $(lib_LIBRARIES) -a ! -s $(lib_LIBRARIES) \
                -a -n "$$TE_WIN32_BUILD_HOST" ; then \
            rm $(lib_LIBRARIES) ; $(MAKE) install ; \
        fi

EXTRA_DIST=\
	aio.c \
	fcntl.c \
	netdb.c \
	net_if_arp.c \
	net_if.c \
	signal.c \
	sys_poll.c \
	sys_resource.c \
	sys_stat.c \
	sys_wait.c 
        
else

lib_LTLIBRARIES=librpc_types.la

librpc_types_la_SOURCES = defs.c 

nodist_librpc_types_la_SOURCES = \
	sys_socket_conv.c \
	aio_conv.c \
	fcntl_conv.c \
	netdb_conv.c \
	net_if_arp_conv.c \
	net_if_conv.c \
	signal_conv.c \
	sys_poll_conv.c \
	sys_resource_conv.c \
	sys_stat_conv.c \
	sys_wait_conv.c \
	wsa_conv.c

CLEANFILES=$(nodist_librpc_types_la_SOURCES)

EXTRA_DIST=\
	sys_socket.c \
	aio.c \
	fcntl.c \
	netdb.c \
	net_if_arp.c \
	net_if.c \
	signal.c \
	sys_poll.c \
	sys_resource.c \
	sys_stat.c \
	sys_wait.c \
	wsa.c

%_conv.c: %.c
	$(srcdir)/../../engine/builder/te_rpc_conv $^ $@

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

install-exec-hook:
	touch -c -r librpc_types.* $(DESTDIR)/$(libdir)/librpc_types.*

all: install

endif
