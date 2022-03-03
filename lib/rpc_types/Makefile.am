
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) $(TE_CPPFLAGS_VAR)
AM_CFLAGS = @TE_CFLAGS@ $(TIRPC_CFLAGS) $(TE_CFLAGS_VAR)

include $(top_srcdir)/../../auxdir/rpcgen.am

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
	te_rpc_sys_mman.h \
	te_rpc_wsa.h \
	te_rpc_sysconf.h \
	te_rpc_dlfcn.h \
	te_rpc_dirent.h \
	te_rpc_pthread.h

if WINDOWS

lib_LIBRARIES=librpc_types.a

librpc_types_a_CFLAGS=-DTE_DUMMY $(AM_CFLAGS)
librpc_types_a_SOURCES=defs.c wsa.c.rpch sys_socket.c.rpch fcntl.c.rpch
EXTRA_librpc_types_a_DEPENDENCIES = $(RPCGEN_EXTRA_PREREQS)

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
	aio.c.rpch \
	fcntl.c.rpch \
	netdb.c.rpch \
	net_if_arp.c.rpch \
	net_if.c.rpch \
	signal.c.rpch \
	sys_poll.c.rpch \
	sys_epoll.c.rpch \
	sys_resource.c.rpch \
	sys_stat.c.rpch \
	sys_wait.c.rpch \
	sysconf_name.c.rpch

else

lib_LTLIBRARIES=librpc_types.la

librpc_types_la_SOURCES = defs.c \
	sys_socket.c.rpch \
	aio.c.rpch \
	fcntl.c.rpch \
	netdb.c.rpch \
	net_if_arp.c.rpch \
	net_if.c.rpch \
	linux_net_tstamp.c.rpch \
	signal.c.rpch \
	sys_poll.c.rpch \
	sys_epoll.c.rpch \
	sys_resource.c.rpch \
	sys_systeminfo.c.rpch \
	sys_stat.c.rpch \
	sys_wait.c.rpch \
	sys_mman.c.rpch \
	wsa.c.rpch \
	sysconf_name.c.rpch \
	dlfcn.c.rpch \
	dirent.c.rpch \
	pthread.c.rpch \
	sys_time.c.rpch

librpc_types_la_LIBADD = $(TIRPC_LIBS)

EXTRA_librpc_types_la_DEPENDENCIES = $(RPCGEN_EXTRA_PREREQS)

BUILT_SOURCES=\
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
	sys_mman.c \
	wsa.c \
	sysconf_name.c \
	dlfcn.c \
	dirent.c \
	sys_time.c

CLEANFILES = $(BUILT_SOURCES)

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

install-exec-hook:
	touch -c -r librpc_types.* $(DESTDIR)/$(libdir)/librpc_types.*

all: install

endif
