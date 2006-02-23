
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@

include_HEADERS=conf_oid.h 

lib_LTLIBRARIES=libconf_oid.la

libconf_oid_la_SOURCES=conf_oid.c

install-exec-hook:
	touch -c -r libconf_oid.la $(DESTDIR)/$(libdir)/libconf_oid.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
		touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install

