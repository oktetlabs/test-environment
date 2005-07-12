
AM_CPPFLAGS=@TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include
AM_CFLAGS =
AM_LDFLAGS=@TE_LDFLAGS@ 

include_HEADERS = tce_collector.h
lib_LIBRARIES = libtce.a

libtce_a_SOURCES = tce_collector.c kernel.c tce_internal.h gcov-io.h posix_tar.h

install-exec-hook:
	touch -c -r libtce.a $(DESTDIR)/$(prefix)/lib/libtce.a

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install
