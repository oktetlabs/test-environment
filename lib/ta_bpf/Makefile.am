ACLOCAL_AMFLAGS = -I @TE_M4@

include_HEADERS = te_bpf_helpers.h

bin_SCRIPTS = te_build_bpf

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install
