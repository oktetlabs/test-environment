
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I. -I$(DESTDIR)/$(includedir) -include config.h
AM_CFLAGS = @TE_CFLAGS@ -Wno-unused-parameter -Wno-unused-variable \
            -fno-strict-aliasing


include_HEADERS=rpc_xdr.h tarpc.h


if WINDOWS

lib_LIBRARIES=librpcxdrta.a

librpcxdrta_a_SOURCES=xml_xdr.c rpc_xdr.c tarpc_xdr.c 

BUILT_SOURCES=tarpc_xdr.c 

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

lib_LTLIBRARIES=librpcxdr.la

librpcxdr_la_SOURCES=xml_xdr.c rpc_xdr.c

nodist_librpcxdr_la_SOURCES=tarpc_xdr.c tarpc_dummy.c

lib_LIBRARIES=librpcxdrta.a

librpcxdrta_a_CFLAGS=-DTE_DUMMY $(AM_CFLAGS)
librpcxdrta_a_SOURCES=xml_xdr.c rpc_xdr.c

nodist_librpcxdrta_a_SOURCES=tarpc_xdr.c 

install-exec-hook:
	touch -c -r librpcxdr.la $(DESTDIR)/$(libdir)/librpcxdr.*
	touch -c -r librpcxdr.la $(DESTDIR)/$(libdir)/librpcxdrta.*

install-data-hook:
	touch -c -r $(srcdir)/rpc_xdr.h \
                    $(DESTDIR)/$(includedir)/rpc_xdr.h 
	touch -c -r $(srcdir)/../../include/tarpc.x.m4 \
                    $(DESTDIR)/$(includedir)/tarpc.h

all: install

endif

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
	cat tarpc.h | grep -v "#define _.* " | grep -v '<pthread.h>' > tarpc.h.tmp ; 
	mv tarpc.h.tmp tarpc.h
	rpcgen -c -M tarpc.x -o tarpc_xdr.c
	$(srcdir)/../../engine/builder/te_generate_rpctbl tmp.x.m4 --generate-dummy
	rm -f tarpc.x tmp.x.m4

