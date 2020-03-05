
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) $(TE_CPPFLAGS_VAR)
AM_CFLAGS = @TE_CFLAGS@ $(TE_CFLAGS_VAR)

include_HEADERS=conf_api.h conf_types.h conf_messages.h conf_ipc.h

lib_LTLIBRARIES=libconfapi.la libconf_types.la libconf_ipc.la

libconf_types_la_SOURCES=conf_types.c
libconf_ipc_la_SOURCES=conf_ipc.c
libconfapi_la_SOURCES=conf_api.c conf_types.c conf_ipc.c

install-exec-hook:
	touch -c -r libconf_types.la $(DESTDIR)/$(libdir)/libconf_types.*
	touch -c -r libconf_ipc.la $(DESTDIR)/$(libdir)/libconf_ipc.*
	touch -c -r libconfapi.la $(DESTDIR)/$(libdir)/libconfapi.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
		touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install

