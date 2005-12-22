
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@ 
AM_LDFLAGS = @TE_LDFLAGS@ 


LIBTOOL_DEPS = @LIBTOOL_DEPS@
libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

include_HEADERS = comm_net_engine.h

lib_LTLIBRARIES = libcomm_net_engine.la

libcomm_net_engine_la_SOURCES = comm_net_engine.c


install-exec-hook:
	touch -c -r .libs/libcomm_net_engine.la \
		$(DESTDIR)/$(libdir)/libcomm_net_engine.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install
