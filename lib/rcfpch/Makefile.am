
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)

# -Wno-uninitialized is used to suppress warnings:
#   variable '...' might be clobbered by `longjmp' or `vfork'
# TODO: Investigate whether it can cause any issues.
AM_CFLAGS = @TE_CFLAGS@ -Wno-uninitialized

include_HEADERS=rcf_ch_api.h rcf_pch.h rcf_pch_ta_cfg.h rcf_pch_mem.h \
                rpc_transport.h
noinst_HEADERS=rcf_pch_internal.h 

lib_LIBRARIES=librcfpch.a

if WINDOWS

librcfpch_a_SOURCES=rcf_pch_mem.c rpc_transport.c

librcfpch.a: $(librcfpch_a_SOURCES) 
	TE_INSTALL=$(DESTDIR)/$(prefix)/.. \
        $(TE_PATH)/te_build_win_lib $(TE_PLATFORM) $@ \
                                    $(DESTDIR)/$(prefix) $^

install-exec-hook:
	if test -e $(lib_LIBRARIES) -a ! -s $(lib_LIBRARIES) \
                -a -n "$$TE_WIN32_BUILD_HOST" ; then \
            rm $(lib_LIBRARIES) ; $(MAKE) install ; \
        fi

EXTRA_DIST=rcf_pch.c rcf_pch_rpc.c rcf_pch_file.c rcf_pch_conf.c \
           rcf_pch_var.c rcf_pch_ta_cfg.c rcf_pch_rpc_server.c

else

librcfpch_a_SOURCES=rcf_pch.c rcf_pch_rpc.c rcf_pch_mem.c \
                    rcf_pch_file.c rcf_pch_conf.c rcf_pch_var.c \
                    rcf_pch_ta_cfg.c rpc_transport.c 

install-exec-hook:
	touch -c -r librcfpch.* $(DESTDIR)/$(libdir)/librcfpch.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install

EXTRA_DIST=

endif

EXTRA_DIST+=rcf_pch_rpc_server.c
