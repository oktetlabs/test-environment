
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@ 
AM_LDFLAGS = @TE_LDFLAGS@ 


include_HEADERS=te_builder_ts.h

lib_LTLIBRARIES=libbsapi.la

libbsapi_la_SOURCES=bsapi.c


install-exec-hook:
	touch -c -r libbsapi.la $(DESTDIR)/$(libdir)/libbsapi.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install
