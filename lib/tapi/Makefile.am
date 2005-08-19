
AM_CPPFLAGS = -I$(DESTDIR)/$(prefix)/include
AM_LDFLAGS = @TE_LDFLAGS@ 

AM_YFLAGS = -d

include_HEADERS = tapi_test.h tapi_bufs.h tapi_env.h tapi_cfg.h \
                  tapi_cfg_net.h tapi_cfg_arl.h tapi_cfg_base.h \
                  tapi_sockaddr.h tapi_file.h tapi_jmp.h tapi_cfg_dhcp.h

lib_LTLIBRARIES = libtapi.la

libtapi_la_SOURCES = tapi_bufs.c tapi_env.c env_lex.l env_gram.y \
                     tapi_cfg.c tapi_cfg_net.c tapi_cfg_arl.c \
                     tapi_cfg_base.c tapi_sockaddr.c test_params.c \
                     tapi_file.c tapi_jmp.c tapi_cfg_dhcp.c 

BUILT_SOURCES = env_gram.h


install-exec-hook:
	touch -c -r .libs/libtapi.la $(DESTDIR)/$(prefix)/lib/libtapi.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install
