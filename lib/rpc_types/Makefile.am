
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) $(TE_CPPFLAGS_VAR)
AM_CFLAGS = @TE_CFLAGS@ $(TE_CFLAGS_VAR)


include_HEADERS = \
	te_rpc_types.h \
	te_rpc_defs.h \
	te_rpc_aio.h \
	te_rpc_errno.h \
	te_rpc_fcntl.h \
	te_rpc_linux_net_tstamp.h \
	te_rpc_netdb.h \
	te_rpc_net_if_arp.h \
	te_rpc_net_if.h \
	te_rpc_signal.h \
	te_rpc_sys_poll.h \
	te_rpc_sys_epoll.h \
	te_rpc_sys_resource.h \
	te_rpc_sys_systeminfo.h \
	te_rpc_sys_time.h \
	te_rpc_sys_socket.h \
	te_rpc_sys_stat.h \
	te_rpc_sys_wait.h \
	te_rpc_wsa.h \
	te_rpc_sysconf.h \
        te_rpc_dlfcn.h \
        te_rpc_dirent.h

if WINDOWS

lib_LIBRARIES=librpc_types.a

librpc_types_a_CFLAGS=-DTE_DUMMY $(AM_CFLAGS)
librpc_types_a_SOURCES=defs.c wsa.c sys_socket.c fcntl.c

librpc_types.a: $(librpc_types_a_SOURCES) 
	TE_INSTALL=$(DESTDIR)/$(prefix)/.. \
        PATH="$${PATH}:$(TE_BASE)/engine/builder:$(TE_PATH)" \
        te_build_win_lib $(TE_PLATFORM) $@ $(DESTDIR)/$(prefix) $^

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
	sys_epoll.c \
	sys_resource.c \
	sys_stat.c \
	sys_wait.c \
	sysconf_name.c

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
	linux_net_tstamp_conv.c \
	signal_conv.c \
	sys_poll_conv.c \
	sys_epoll_conv.c \
	sys_resource_conv.c \
	sys_systeminfo_conv.c \
	sys_stat_conv.c \
	sys_wait_conv.c \
	wsa_conv.c \
	sysconf_name_conv.c \
        dlfcn_conv.c \
        dirent.c

CLEANFILES=$(nodist_librpc_types_la_SOURCES)

EXTRA_DIST=\
	sys_socket.c \
	aio.c \
	fcntl.c \
	netdb.c \
	net_if_arp.c \
	net_if.c \
	linux_net_tstamp.c \
	signal.c \
	sys_poll.c \
	sys_epoll.c \
	sys_resource.c \
	sys_systeminfo.c \
	sys_stat.c \
	sys_wait.c \
	wsa.c \
	sysconf_name.c \
    dlfcn.c

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
