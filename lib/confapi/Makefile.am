
AUTOMAKE_OPTIONS = foreign 

AM_CFLAGS=@TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include \
                      -I$(DESTDIR)/$(prefix)/../include
AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 

include_HEADERS=conf_api.h conf_types.h conf_messages.h

lib_LIBRARIES=libconfapi.a libconf_types.a 

libconfapi_a_SOURCES=conf_api.c conf_types.c 
libconf_types_a_SOURCES=conf_types.c

install-exec-hook:
	touch -c -r libconfapi.a $(DESTDIR)/$(prefix)/lib/libconfapi.a
	touch -c -r libconf_types.a $(DESTDIR)/$(prefix)/lib/libconf_types.a

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install

