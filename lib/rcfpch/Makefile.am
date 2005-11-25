
AM_CPPFLAGS = @TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include 
AM_LDFLAGS = @TE_LDFLAGS@ 

include_HEADERS=rcf_ch_api.h rcf_pch.h rcf_pch_ta_cfg.h rcf_pch_mem.h
noinst_HEADERS=rcf_pch_internal.h

lib_LIBRARIES=librcfpch.a

librcfpch_a_SOURCES=rcf_pch.c rcf_pch_rpc.c rcf_pch_mem.c \
                    rcf_pch_file.c rcf_pch_conf.c rcf_pch_var.c \
                    rcf_pch_ta_cfg.c
                    

install-exec-hook:
	touch -c -r librcfpch.a $(DESTDIR)/$(prefix)/lib/librcfpch.a

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install
