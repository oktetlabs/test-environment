
AM_CPPFLAGS = -I$(DESTDIR)/$(prefix)/include \
              -I$(DESTDIR)/$(prefix)/../include
AM_CFLAGS = 

AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 


include_HEADERS=rcf_rpc.h

lib_LIBRARIES=librcfrpc.a

librcfrpc_a_SOURCES=rcf_rpc.c tarpc_xdr.c

CLEANFILES=tarpc.x tarpc.h

rcf_rpc.o: tarpc.h

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
	touch -c -r librcfrpc.a $(DESTDIR)/$(prefix)/lib/librcfrpc.a

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install

