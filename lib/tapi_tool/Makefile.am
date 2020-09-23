
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) $(TE_CPPFLAGS_VAR)

AM_CFLAGS = @TE_CFLAGS@ $(TE_CFLAGS_VAR)

AM_LDFLAGS = -L$(DESTDIR)/$(libdir)

include_HEADERS= \
	tapi_mke2fs.h \
	tapi_netperf.h \
	tapi_nptcp.h \
	tapi_ping.h \
	tapi_sfnt_pingpong.h \
	tapi_stress.h \
	tapi_wrk.h

lib_LTLIBRARIES = libtapi_tool.la

libtapi_tool_la_SOURCES = \
	tapi_mke2fs.c \
	tapi_netperf.c \
	tapi_nptcp.c \
	tapi_ping.c \
	tapi_sfnt_pingpong.c \
	tapi_stress.c \
	tapi_wrk.c

install-exec-hook:
	for i in ${lib_LTLIBRARIES} ; do \
	    touch -c -r $$i $(DESTDIR)/$(prefix)/lib/$$i ; \
	done

install-data-hook:
	for i in $(include_HEADERS) ; do \
		touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install
