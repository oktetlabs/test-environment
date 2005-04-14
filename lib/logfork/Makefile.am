
AUTOMAKE_OPTIONS = foreign 

AM_CPPFLAGS=@TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include
AM_CFLAGS =
AM_LDFLAGS=@TE_LDFLAGS@ 

include_HEADERS=logfork.h ta_logfork.h

lib_LIBRARIES=liblogfork.a

liblogfork_a_SOURCES=logfork.c

install-exec-hook:
	touch -c -r liblogfork.a $(DESTDIR)/$(prefix)/lib/liblogfork.a

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install

