
AM_CPPFLAGS = -I$(DESTDIR)/$(prefix)/include
AM_LDFLAGS = @TE_LDFLAGS@ 

include_HEADERS = \
	te_tools.h \
	te_bufs.h \
	te_shell_cmd.h \
	te_log_fmt.h \
	te_format.h \
	te_iscsi.h

lib_LIBRARIES = libtools.a

libtools_a_SOURCES = \
	te_bufs.c \
	te_shell_cmd.c \
	te_log_fmt.c \
	te_format.c


install-exec-hook:
	touch -c -r libtools.a $(DESTDIR)/$(prefix)/lib/libtools.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install
