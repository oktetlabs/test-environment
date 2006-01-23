
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I. -I$(DESTDIR)/$(includedir) -include config.h
AM_CFLAGS = @TE_CFLAGS@ -Wno-unused-parameter -Wno-unused-variable
include_HEADERS=rpc_xdr.h tarpc.h

lib_LIBRARIES=librpcxdr.a librpcxdrta.a

librpcxdr_a_SOURCES=xml_xdr.c rpc_xdr.c

nodist_librpcxdr_a_SOURCES=tarpc_xdr.c tarpc_dummy.c

librpcxdrta_a_SOURCES=xml_xdr.c rpc_xdr.c

nodist_librpcxdrta_a_SOURCES=tarpc_xdr.c 

CLEANFILES=tarpc.h tarpc_xdr.c tarpc_dummy.c

if DIST_RPCDEFS

EXTRA_DIST=@RPCDEFS_DIST_FILES@

@RPCDEFS_DIST_FILES@: @RPCDEFS@
	cp @RPCDEFS@ .

endif

tarpc.h tarpc_xdr.c tarpc_dummy.c: $(srcdir)/../../include/tarpc.x.m4 @RPCDEFS@
	rm -f tarpc.h tarpc_xdr.c
	cat $^ > tmp.x.m4
	m4 tmp.x.m4 > tarpc.x
	rpcgen -h -M tarpc.x -o tarpc.h
	rpcgen -c -M tarpc.x -o tarpc_xdr.c
	$(srcdir)/../../engine/builder/te_generate_rpctbl tmp.x.m4 --generate-dummy
	rm -f tarpc.x tmp.x.m4

install-exec-hook:
	touch -c -r librpcxdr.a $(DESTDIR)/$(libdir)/librpcxdr.a

install-data-hook:
	touch -c -r $(srcdir)/rpc_xdr.h \
                    $(DESTDIR)/$(includedir)/rpc_xdr.h 
	touch -c -r $(srcdir)/../../include/tarpc.x.m4 \
                    $(DESTDIR)/$(includedir)/tarpc.h

all: install
