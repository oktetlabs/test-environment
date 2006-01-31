
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@ 

include_HEADERS=rcf_ch_api.h rcf_pch.h rcf_pch_ta_cfg.h rcf_pch_mem.h
noinst_HEADERS=rcf_pch_internal.h

if WINDOWS

RANLIB=touch

lib_LIBRARIES=librcfpch.lib

librcfpch_lib_SOURCES=rcf_pch_mem.c 

librcfpch.lib: $(librcfpch_lib_SOURCES) 
	$(srcdir)/../../engine/builder/te_build_win_lib $(srcdir) @host@ $@ \
        $(DESTDIR)/$(prefix) $(librcfpch_lib_SOURCES)

EXTRA_DIST=rcf_pch.c rcf_pch_rpc.c rcf_pch_file.c rcf_pch_conf.c \
           rcf_pch_var.c rcf_pch_ta_cfg.c

else

lib_LIBRARIES=librcfpch.a

librcfpch_a_SOURCES=rcf_pch.c rcf_pch_rpc.c rcf_pch_mem.c \
                    rcf_pch_file.c rcf_pch_conf.c rcf_pch_var.c \
                    rcf_pch_ta_cfg.c

EXTRA_DIST=

endif

EXTRA_DIST+=rcf_pch_rpc_server.c                    

install-exec-hook:
	touch -c -r librcfpch.* $(DESTDIR)/$(libdir)/librcfpch.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install
