
AUTOMAKE_OPTIONS = foreign 

AM_CPPFLAGS=@TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include 
AM_CFLAGS =
AM_LDFLAGS=@TE_LDFLAGS@ 

include_HEADERS=conf_api.h conf_types.h conf_messages.h

lib_LTLIBRARIES=libconfapi.la libconf_types.la

libconfapi_la_SOURCES=conf_api.c conf_types.c 
libconf_types_la_SOURCES=conf_types.c

install-exec-hook:
	touch -c -r .libs/libconfapi.la $(DESTDIR)/$(prefix)/lib/libconfapi.*
	touch -c -r .libs/libconf_types.la $(DESTDIR)/$(prefix)/lib/libconf_types.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install

