
AM_CPPFLAGS = -I$(DESTDIR)/$(prefix)/include \
              -I$(DESTDIR)/$(prefix)/../include

# TE_CFLAGS are already included in CFLAGS in configure.ac
AM_CFLAGS =

AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 

include_HEADERS = tapi_tad.h tapi_cfg.h tapi_cfg_net.h tapi_cfg_arl.h \
                  tapi_cfg_base.h tapi_sockaddr.h

lib_LIBRARIES = libtapi.a

libtapi_a_SOURCES = tapi_tad.c tapi_cfg.c tapi_cfg_net.c tapi_cfg_arl.c \
                    tapi_cfg_base.c tapi_sockaddr.c


install-exec-hook:
	for i in $(lib_LIBRARIES) ; do \
	    touch -c -r $$i $(DESTDIR)/$(prefix)/lib/$$i ; \
	done

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install
