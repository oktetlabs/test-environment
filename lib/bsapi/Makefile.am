
AUTOMAKE_OPTIONS = foreign 

AM_CFLAGS=@TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include \
                      -I$(DESTDIR)/$(prefix)/../include
AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 

include_HEADERS=te_builder_nuts.h te_builder_ts.h

lib_LIBRARIES=libbsapi.a

libbsapi_a_SOURCES=bsapi.c

install-exec-hook:
	touch -c -r libbsapi.a $(DESTDIR)/$(prefix)/lib/libbsapi.a

all: install

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done
