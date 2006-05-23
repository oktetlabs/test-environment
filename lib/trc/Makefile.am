
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@

include_HEADERS = \
	te_trc.h

#lib_LTLIBRARIES = libtrc.la
#
#libtools_la_SOURCES = \
#	te_format.c

#install-exec-hook:
#	touch -c -r libtools.la $(DESTDIR)/$(libdir)/libtools.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
		touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install
