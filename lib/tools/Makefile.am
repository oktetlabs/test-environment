
AM_CPPFLAGS = -I$(DESTDIR)/$(prefix)/include
AM_LDFLAGS = @TE_LDFLAGS@ 

include_HEADERS = \
	te_tools.h \
	te_bufs.h

lib_LIBRARIES = libtools.a

libtools_a_SOURCES = \
	te_bufs.c


install-exec-hook:
	touch -c -r libtools.a $(DESTDIR)/$(prefix)/lib/libtools.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install
