
AM_CPPFLAGS = -I$(DESTDIR)/$(prefix)/include \
              -I$(DESTDIR)/$(prefix)/../include

# TE_CFLAGS are already included in CFLAGS in configure.ac
AM_CFLAGS =

AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 

include_HEADERS=rcf_ch_api.h rcf_pch.h
noinst_HEADERS=rcf_pch_internal.h

lib_LIBRARIES=librcfpch.a

librcfpch_a_SOURCES=rcf_pch.c rcf_pch_file.c rcf_pch_conf.c rcf_pch_var.c

install-exec-hook:
	touch -c -r librcfpch.a $(DESTDIR)/$(prefix)/lib/librcfpch.a

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install
