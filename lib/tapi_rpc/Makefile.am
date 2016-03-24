
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) $(TE_CPPFLAGS_VAR)
AM_CFLAGS = @TE_CFLAGS@ $(TE_CFLAGS_VAR)
AM_LDFLAGS = -L$(DESTDIR)/$(libdir)


include_HEADERS= \
    tapi_rpc.h \
    tapi_rpc_aio.h \
    tapi_rpc_ifnameindex.h \
    tapi_rpc_misc.h \
    tapi_rpc_netdb.h \
    tapi_rpc_signal.h \
    tapi_rpc_socket.h \
    tapi_rpc_stdio.h \
    tapi_rpc_telephony.h \
    tapi_rpc_unistd.h \
    tapi_rpc_winsock2.h \
    tapi_rpc_client_server.h \
    tapi_rpcsock_macros.h \
    tapi_rpc_params.h \
    tapi_rpc_internal.h \
    tapi_rpc_power_sw.h \
    tapi_rpc_dlfcn.h \
    tapi_rpc_dirent.h

lib_LTLIBRARIES=libtapi_rpc.la

libtapi_rpc_la_SOURCES= \
    aio.c \
    client_server.c \
    ifnameindex.c \
    ioctl.c \
    misc.c \
    netdb.c \
    signal.c \
    socket.c \
    stdio.c \
    telephony.c \
    unistd.c \
    wsa.c \
    power_sw.c \
    dlfcn.c \
    pipe.c \
    dirent.c \
    tapi_rpc_internal.c


install-exec-hook:
	touch -c -r libtapi_rpc.la $(DESTDIR)/$(libdir)/libtapi_rpc.*

install-data-hook: 
	for i in $(include_HEADERS) ; do \
		touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install
