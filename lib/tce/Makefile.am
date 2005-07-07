
AUTOMAKE_OPTIONS = foreign 

AM_CPPFLAGS=@TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include
AM_CFLAGS =
AM_LDFLAGS=@TE_LDFLAGS@ 

lib_LIBRARIES=libtce.a
bin_PROGRAMS=tce_collector tce_dump

libtce_a_SOURCES=tce_collector.c gcov-io.h posix_tar.h

tce_collector_SOURCES=main.c tce_collector.c

tce_dump_SOURCES=tce_dump.c tce_collector.c
tce_dump_LDADD = -lrcfapi -lrpcxdr -llogger_ten -lipc -lpthread
tce_dump_DEPENDENCIES = $(DESTDIR)/$(prefix)/lib/librcfapi.a \
                        $(DESTDIR)/$(prefix)/lib/liblogger_ten.a \
                        $(DESTDIR)/$(prefix)/lib/libipc.a

dist_bin_SCRIPTS=tce_kallsyms

install-exec-hook:
	touch -c -r libtce.a $(DESTDIR)/$(prefix)/lib/libtce.a

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install

