
SUBDIRS = . tests

ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@ $(SNMP_F)


lib_LIBRARIES=libasn.a

include_HEADERS=asn_usr.h asn_impl.h

libasn_a_SOURCES=asn_val.c asn_text.c 

install-exec-hook:
	touch -c -r libasn.a $(DESTDIR)/$(libdir)/libasn.a 

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install

