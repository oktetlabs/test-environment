
AM_CPPFLAGS=-I$(DESTDIR)/$(prefix)/include -I$(DESTDIR)/$(prefix)/../include

AM_LDFLAGS=-L$(DESTDIR)/$(prefix)/lib


include_HEADERS=tapi_rpcsock.h tapi_rpcsock_macros.h

lib_LTLIBRARIES=libtapi_rpcsock.la

libtapi_rpcsock_la_SOURCES=tapi_rpcsock.c tarpc_xdr.c


CLEANFILES=tarpc.x tarpc.h tarpc_xdr.c

tapi_rpcsock.lo: tarpc.h

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
	touch -c -r .libs/libtapi_rpcsock.la \
		$(DESTDIR)/$(prefix)/lib/libtapi_rpcsock.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install
