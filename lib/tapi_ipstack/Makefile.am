
AUTOMAKE_OPTIONS = foreign 

AM_CFLAGS=@TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include \
                      -I$(DESTDIR)/$(prefix)/../include

AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 


lib_LIBRARIES=libtapi_ipstack.a

include_HEADERS=tapi_ipstack.h
libtapi_ipstack_a_SOURCES=tapi_ipstack.c

#VPATH = @srcdir@:@srcdir@/tests

#check_PROGRAMS=mib_table1

#LDFLAGS=-static

#CFLAGS=@CFLAGS@ -Wall


install-exec-hook:
	touch -c -r libtapi_ipstack.a $(DESTDIR)/$(prefix)/lib/libtapi_ipstack.a

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install

