
AM_CPPFLAGS=-I$(DESTDIR)/$(prefix)/include 
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
    tapi_rpcsock_macros.h

noinst_HEADERS=tapi_rpc_internal.h


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
    wsa.c \
    tarpc_xdr.c


CLEANFILES=tarpc.x tarpc.h tarpc_xdr.c

tapi_rpc.lo: tarpc.h

tarpc.x: @RPC_XFILE@
	m4 @RPC_XFILE@ > tarpc.x

tarpc.h: tarpc.x
	$(RM) -f tarpc.h
	rpcgen -h -M tarpc.x -o tarpc.h

tarpc_xdr.c: tarpc.h
	$(RM) -f tarpc_xdr.c
	rpcgen -c -M tarpc.x -o tarpc_xdr.c
	awk '\
            /buf;/ { print $0 ; printf("\t(void)buf;") ; next ; } \
            { print $0 ; next ;}' tarpc_xdr.c > tarpc_xdr.c.new
	mv tarpc_xdr.c.new tarpc_xdr.c

install-exec-hook:
	touch -c -r .libs/libtapi_rpc.la \
		$(DESTDIR)/$(prefix)/lib/libtapi_rpc.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install
