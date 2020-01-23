
include $(top_srcdir)/../../auxdir/rpcgen.am

ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ $(TIRPC_CFLAGS) -I. -I$(DESTDIR)/$(includedir) \
              $(TE_CPPFLAGS_VAR)

AM_CFLAGS = @TE_CFLAGS@ -Wno-unused-parameter -Wno-unused-variable \
            -fno-strict-aliasing $(TE_CFLAGS_VAR)


include_HEADERS=rpc_xdr.h
nodist_include_HEADERS=tarpc.h
pkgdata_DATA=tarpc.x

if WINDOWS

lib_LIBRARIES=librpcxdrta.a

librpcxdrta_a_SOURCES=xml_xdr.c rpc_xdr.c rpc_lookup.c
nodist_librpcxdrta_a_SOURCES = tarpc.x.m4 tarpc_rpctbl.c
EXTRA_librpcxdrta_a_DEPENDENCIES = $(RPCGEN_EXTRA_PREREQS)

librpcxdrta.a: $(librpcxdrta_a_SOURCES) 
	TE_INSTALL=$(DESTDIR)/$(prefix)/.. \
        PATH="$${PATH}:$(TE_BASE)/engine/builder:$(TE_PATH)" \
        te_build_win_lib $(TE_PLATFORM) $@ $(DESTDIR)/$(prefix) $^

install-exec-hook:
	if test -e $(lib_LIBRARIES) -a ! -s $(lib_LIBRARIES) \
	        -a -n "$$TE_WIN32_BUILD_HOST" ; then \
	    rm $(lib_LIBRARIES) ; $(MAKE) install ; \
	fi

else

lib_LTLIBRARIES=librpcxdr.la librpcxdrta.la

librpcxdr_la_CFLAGS=-DTE_RPC_CLIENT $(AM_CFLAGS)
librpcxdr_la_SOURCES=xml_xdr.c rpc_xdr.c
librpcxdr_la_LIBADD = $(TIRPC_LIBS)

nodist_librpcxdr_la_SOURCES=tarpc_xdr.c tarpc_rpctbl.c

EXTRA_librpcxdr_la_DEPENDENCIES = $(RPCGEN_EXTRA_PREREQS)

lib_LIBRARIES=librpcxdrta.a

librpcxdrta_la_CFLAGS=-DTE_DUMMY $(AM_CFLAGS)
librpcxdrta_la_SOURCES=xml_xdr.c rpc_xdr.c
librpcxdrta_la_LIBADD = $(TIRPC_LIBS)

nodist_librpcxdrta_la_SOURCES=tarpc_xdr.c tarpc_rpctbl.c tarpc_stub.c

EXTRA_librpcxdrta_la_DEPENDENCIES = $(RPCGEN_EXTRA_PREREQS)

install-exec-hook:
	touch -c -r librpcxdr.la $(DESTDIR)/$(libdir)/librpcxdr.*
	touch -c -r librpcxdrta.la $(DESTDIR)/$(libdir)/librpcxdrta.*

install-data-hook:
	touch -c -r $(srcdir)/rpc_xdr.h \
                    $(DESTDIR)/$(includedir)/rpc_xdr.h 
	touch -c -r $(srcdir)/tarpc_base.x.m4 \
                    $(DESTDIR)/$(includedir)/tarpc.h

all: install

endif

BUILT_SOURCES=tarpc_xdr.c tarpc_rpctbl.c tarpc_stub.c tarpc.h tarpc.x.m4

CLEANFILES=$(BUILT_SOURCES)

if DIST_RPCDEFS

EXTRA_DIST=@RPCDEFS_DIST_FILES@

@RPCDEFS_DIST_FILES@: @RPCDEFS@
	cp @RPCDEFS@ .

endif

# This is a temporary hack which will go as soon as RPC library is
# modularized
tarpc.x.m4: $(srcdir)/tarpc_base.x.m4 @RPCDEFS@
	cat $^ >$@

