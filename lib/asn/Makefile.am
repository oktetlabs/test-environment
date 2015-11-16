
SUBDIRS = . tests

ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) $(TE_CPPFLAGS_VAR)
AM_CFLAGS = @TE_CFLAGS@ $(SNMP_F) $(TE_CFLAGS_VAR)


lib_LTLIBRARIES=libasn.la

include_HEADERS=asn_usr.h asn_impl.h

libasn_la_SOURCES=asn_val.c asn_text.c 

install-exec-hook:
	touch -c -r libasn.la $(DESTDIR)/$(libdir)/libasn.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
		touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install

