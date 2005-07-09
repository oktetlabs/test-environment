
SUBDIRS = . tests

AM_CPPFLAGS=@TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include
AM_CFLAGS=$(SNMP_F)
AM_LDFLAGS=@TE_LDFLAGS@ 


lib_LIBRARIES=libasn.a

include_HEADERS=asn_usr.h asn_impl.h

libasn_a_SOURCES=asn_val.c asn_text.c 

install-exec-hook:
	touch -c -r libasn.a $(DESTDIR)/$(prefix)/lib/libasn.a 

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install

