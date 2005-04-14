
AUTOMAKE_OPTIONS = foreign 

AM_CPPFLAGS=@TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include
AM_CFLAGS =
AM_LDFLAGS=@TE_LDFLAGS@ 

include_HEADERS=conf_oid.h 

lib_LIBRARIES=libconf_oid.a

libconf_oid_a_SOURCES=conf_oid.c

install-exec-hook:
	touch -c -r libconf_oid.a $(DESTDIR)/$(prefix)/lib/libconf_oid.a

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install

