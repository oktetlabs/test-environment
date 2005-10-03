
AM_CPPFLAGS = -I$(DESTDIR)/$(prefix)/include
AM_LDFLAGS = @TE_LDFLAGS@ 

include_HEADERS = \
	tapi_test.h \
	tapi_jmp.h \
	tapi_bufs.h \
	tapi_sockaddr.h \
	tapi_cfg.h \
	tapi_cfg_base.h \
	tapi_cfg_net.h \
	tapi_cfg_arl.h \
	tapi_cfg_dhcp.h \
	tapi_cfg_vtund.h \
	tapi_file.h

lib_LTLIBRARIES = libtapi.la

libtapi_la_SOURCES = \
	test_params.c \
	tapi_jmp.c \
	tapi_bufs.c \
	tapi_sockaddr.c \
	tapi_cfg.c \
	tapi_cfg_base.c \
	tapi_cfg_net.c \
	tapi_cfg_arl.c \
	tapi_cfg_dhcp.c \
	tapi_cfg_vtund.c \
	tapi_file.c


install-exec-hook:
	touch -c -r .libs/libtapi.la $(DESTDIR)/$(prefix)/lib/libtapi.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install
