
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) $(TE_CPPFLAGS_VAR)

AM_CFLAGS = @TE_CFLAGS@ $(TE_CFLAGS_VAR)

AM_LDFLAGS = @TE_LDFLAGS@

include_HEADERS=tapi_bpf.h

lib_LTLIBRARIES=libtapi_bpf.la

libtapi_bpf_la_SOURCES=tapi_bpf.c

install-exec-hook:
	for i in ${lib_LTLIBRARIES} ; do \
	    touch -c -r $$i $(DESTDIR)/$(prefix)/lib/$$i ; \
	done

install-data-hook:
	for i in $(include_HEADERS) ; do \
		touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install
