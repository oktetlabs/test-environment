
AM_CPPFLAGS = @TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include -include config.h
AM_LDFLAGS = @TE_LDFLAGS@ 

include_HEADERS=rpc_xdr.h tarpc.h

lib_LIBRARIES=librpcxdr.a

librpcxdr_a_SOURCES=rpc_xdr.c

nodist_librpcxdr_a_SOURCES=tarpc_xdr.c tarpc_dummy.c

CLEANFILES=tarpc.h tarpc_xdr.c tarpc_dummy.c

tarpc.h tarpc_xdr.c tarpc_dummy.c: $(srcdir)/../../include/tarpc.x.m4
	rm -f tarpc.h tarpc_xdr.c
	m4 $< > tarpc.x
	rpcgen -h -M tarpc.x -o tarpc.h
	rpcgen -c -M tarpc.x -o tarpc_xdr.c
	awk '\
            /buf;/ { print $0 ; printf("\t(void)buf;") ; next ; } \
            { print $0 ; next ;}' tarpc_xdr.c > tarpc_xdr.c.new
	mv tarpc_xdr.c.new tarpc_xdr.c
	$(srcdir)/../../engine/builder/te_generate_rpctbl $< --generate-dummy
	rm -f tarpc.x

install-exec-hook:
	touch -c -r librpcxdr.a $(DESTDIR)/$(prefix)/lib/librpcxdr.a

install-data-hook:
	touch -c -r $(srcdir)/rpc_xdr.h \
                    $(DESTDIR)/$(prefix)/include/rpc_xdr.h 
	touch -c -r $(srcdir)/../../include/tarpc.x.m4 \
                    $(DESTDIR)/$(prefix)/include/tarpc.h

all: install
