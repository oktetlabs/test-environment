
AM_CPPFLAGS=-I$(DESTDIR)/$(prefix)/include 
AM_LDFLAGS=@TE_LDFLAGS@ 


lib_LTLIBRARIES=libtapi_dhcp.la

include_HEADERS=tapi_dhcp.h
libtapi_dhcp_la_SOURCES=tapi_dhcp.c


#VPATH = @srcdir@:@srcdir@/tests

#TESTS=prn01 val_set01 val_set02

#check_PROGRAMS=prn01 prn02 val_set01 val_set02 val_set03 val_set04 \
#		parse00 parse01 parse02 parse03 parse04 ndn_set01

#LDADD=-lasn
#LDFLAGS=-static

#CFLAGS=@CFLAGS@ -Wall


install-exec-hook:
	touch -c -r .libs/libtapi_dhcp.la $(DESTDIR)/$(prefix)/lib/libtapi_dhcp.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install

