
SUBDIRS = .

ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) 
AM_CFLAGS = @TE_CFLAGS@ 
AM_LDFLAGS = -L$(DESTDIR)/$(libdir)


include_HEADERS= tapi_acse.h 

lib_LTLIBRARIES=libtapi_acse.la

libtapi_acse_la_SOURCES= tapi_acse.c 

libtapi_acse_la_LDFLAGS=-lcwmp_data

install-exec-hook:
	touch -c -r libtapi_acse.la $(DESTDIR)/$(libdir)/libtapi_acse.*

install-data-hook: 
	for i in $(include_HEADERS) ; do \
		touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install
