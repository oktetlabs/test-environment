
AM_CPPFLAGS = -I$(DESTDIR)/$(prefix)/include \
              -I$(DESTDIR)/$(prefix)/../include

# TE_CFLAGS are already included in CFLAGS in configure.ac
AM_CFLAGS =

AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 

include_HEADERS = tapi_test.h tapi_bufs.h tapi_cfg.h \
                  tapi_cfg_net.h tapi_cfg_arl.h tapi_cfg_base.h \
                  tapi_sockaddr.h tapi_file.h tapi_jmp.h tapi_cfg_dhcp.h

lib_LTLIBRARIES = libtapi.la

libtapi_la_SOURCES = tapi_bufs.c tapi_cfg.c tapi_cfg_net.c \
                     tapi_cfg_arl.c tapi_cfg_base.c tapi_sockaddr.c \
                     test_params.c tapi_file.c tapi_jmp.c tapi_cfg_dhcp.c


install-exec-hook:
	touch -c -r .libs/libtapi.la $(DESTDIR)/$(prefix)/lib/libtapi.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install
