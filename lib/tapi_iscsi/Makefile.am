
AM_CPPFLAGS=@TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include
AM_LDFLAGS=@TE_LDFLAGS@ 

include_HEADERS=tapi_iscsi.h

lib_LTLIBRARIES=libtapi_iscsi.la

libtapi_iscsi_la_SOURCES=tapi_iscsi.c


install-exec-hook:
	touch -c -r .libs/libtapi_iscsi.la \
		$(DESTDIR)/$(prefix)/lib/libtapi_iscsi.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install
