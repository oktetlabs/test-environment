

AUTOMAKE_OPTIONS = foreign 

AM_CPPFLAGS=-I$(DESTDIR)/$(prefix)/include -I$(DESTDIR)/$(prefix)/../include
AM_LDFLAGS=-L$(DESTDIR)/$(prefix)/lib

include_HEADERS=tapi_rpcsock.h

lib_LIBRARIES=libtapi_rpcsock.a

libtapi_rpcsock_a_SOURCES=tapi_rpcsock.c tarpc_xdr.c

CLEANFILES=tarpc.x tarpc.h

tapi_rpcsock.o: tarpc.h

tarpc.x: @RPC_XFILE@
	m4 @RPC_XFILE@ > tarpc.x

tarpc.h: tarpc.x
	rpcgen -h -M tarpc.x -o tarpc.h

tarpc_xdr.c: tarpc.h
	$(RM) -f tarpc_xdr.c
	rpcgen -c -M tarpc.x -o tarpc_xdr.c
	awk '\
            /buf;/ { print $0 ; printf("\t(void)buf;") ; next ; } \
            { print $0 ; next ;}' tarpc_xdr.c > tarpc_xdr.c.new
	mv tarpc_xdr.c.new tarpc_xdr.c

install-exec-hook:
	touch -c -r libtapi_rpcsock.a $(DESTDIR)/$(prefix)/lib/libtapi_rpcsock.a

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install
