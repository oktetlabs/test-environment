
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@ 

include_HEADERS=rcf_ch_api.h rcf_pch.h rcf_pch_ta_cfg.h rcf_pch_mem.h
noinst_HEADERS=rcf_pch_internal.h

lib_LIBRARIES=librcfpch.a

if WINDOWS

librcfpch_a_SOURCES=rcf_pch_mem.c 

librcfpch.a: $(librcfpch_lib_SOURCES) 
	$(srcdir)/../../engine/builder/te_build_win_lib $(srcdir) @host@ $@ \
        $(DESTDIR)/$(prefix) $(librcfpch_a_SOURCES)

install-exec-hook:
	if test -e $(lib_LIBRARIES) -a ! -s $(lib_LIBRARIES) \
                -a -n "$$TE_WIN32_BUILD_HOST" ; then \
            rm $(lib_LIBRARIES) ; $(MAKE) install ; \
        fi

EXTRA_DIST=rcf_pch.c rcf_pch_rpc.c rcf_pch_file.c rcf_pch_conf.c \
           rcf_pch_var.c rcf_pch_ta_cfg.c

else

librcfpch_a_SOURCES=rcf_pch.c rcf_pch_rpc.c rcf_pch_mem.c \
                    rcf_pch_file.c rcf_pch_conf.c rcf_pch_var.c \
                    rcf_pch_ta_cfg.c

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

