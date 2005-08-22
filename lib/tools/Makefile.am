
AM_CPPFLAGS = -I$(DESTDIR)/$(prefix)/include
AM_LDFLAGS = @TE_LDFLAGS@ 

include_HEADERS = \
	te_tools.h \
	te_bufs.h

lib_LTLIBRARIES = libtools.la

libtools_la_SOURCES = \
	te_bufs.c


install-exec-hook:
	touch -c -r .libs/libtools.la $(DESTDIR)/$(prefix)/lib/libtools.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install
