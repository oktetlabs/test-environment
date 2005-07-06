
AUTOMAKE_OPTIONS = foreign 

AM_CPPFLAGS=@TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include
AM_CFLAGS =
AM_LDFLAGS=@TE_LDFLAGS@ 

lib_LIBRARIES=libtce.a
bin_PROGRAMS=tce_collector

libtce_a_SOURCES=tce_collector.c gcov-io.h posix_tar.h

tce_collector_SOURCES=main.c tce_collector.c

dist_bin_SCRIPTS=tce_kallsyms

install-exec-hook:
	touch -c -r libtce.a $(DESTDIR)/$(prefix)/lib/libtce.a

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install

