
AM_CPPFLAGS=@TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include 
AM_LDFLAGS=-L$(DESTDIR)/$(prefix)/lib


include_HEADERS= \
    tapi_rpc.h \
    tapi_rpc_aio.h \
    tapi_rpc_ifnameindex.h \
    tapi_rpc_misc.h \
    tapi_rpc_netdb.h \
    tapi_rpc_signal.h \
    tapi_rpc_socket.h \
    tapi_rpc_stdio.h \
    tapi_rpc_unistd.h \
    tapi_rpc_winsock2.h \
    tapi_rpcsock_macros.h \
    tapi_rpc_internal.h

lib_LTLIBRARIES=libtapi_rpc.la

libtapi_rpc_la_SOURCES= \
    aio.c \
    ifnameindex.c \
    ioctl.c \
    misc.c \
    netdb.c \
    signal.c \
    socket.c \
    stdio.c \
    unistd.c \
    wsa.c 

install-exec-hook:
	touch -c -r .libs/libtapi_rpc.la \
		$(DESTDIR)/$(prefix)/lib/libtapi_rpc.*

install-data-hook: 
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install
