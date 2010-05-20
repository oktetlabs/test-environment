
AM_CPPFLAGS=-I$(DESTDIR)/$(prefix)/include \
            -I$(DESTDIR)/$(prefix)/../include

AM_CFLAGS=

AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 


noinst_HEADERS=forwarder.h send_queue.h


lib_LIBRARIES=libforwarder.a

if WITH_TAPI
lib_LIBRARIES+=libtapi_forw.a
include_HEADERS=tapi_forw.h
libtapi_forw_a_SOURCES= tapi_forw.c 
else
EXTRA_DIST=tapi_forw.h tapi_forw.c
endif


libforwarder_a_SOURCES= forw_task.c send_queue.c


install-exec-hook:
	for i in ${lib_LIBRARIES} ; do \
	    touch -c -r $$i $(DESTDIR)/$(prefix)/lib/$$i ; \
	done


install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done


all: install
