
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@

include_HEADERS=conf_api.h conf_types.h conf_messages.h

lib_LTLIBRARIES=libconfapi.la libconf_types.la

libconfapi_la_SOURCES=conf_api.c conf_types.c 
libconf_types_la_SOURCES=conf_types.c

install-exec-hook:
	touch -c -r .libs/libconfapi.la $(DESTDIR)/$(libdir)/libconfapi.*
	touch -c -r .libs/libconf_types.la $(DESTDIR)/$(libdir)/libconf_types.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install

