
AM_CPPFLAGS = -I$(DESTDIR)/$(prefix)/include
AM_LDFLAGS = @TE_LDFLAGS@ 
AM_YFLAGS = -d

BUILT_SOURCES =
EXTRA_DIST =

MY_HDRS =
MY_SRCS =


TAPI_ENV_HDRS = tapi_env.h
TAPI_ENV_SRCS = tapi_env.c env_lex.l env_gram.y

if WITH_ENV
MY_HDRS += $(TAPI_ENV_HDRS)
MY_SRCS += $(TAPI_ENV_SRCS)
BUILT_SOURCES += env_gram.h
else
EXTRA_DIST += $(TAPI_ENV_HDRS) $(TAPI_ENV_SRCS)
endif


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
	tapi_file.h \
	$(MY_HDRS)

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
	tapi_file.c \
	$(MY_SRCS)


install-exec-hook:
	touch -c -r .libs/libtapi.la $(DESTDIR)/$(prefix)/lib/libtapi.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install
