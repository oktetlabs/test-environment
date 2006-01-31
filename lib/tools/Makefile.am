
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@

include_HEADERS = \
	te_tools.h \
	te_bufs.h \
	te_shell_cmd.h \
	te_log_fmt.h \
	te_format.h \
	te_iscsi.h

if WINDOWS

lib_LIBRARIES = libtools.lib

libtools_lib_SOURCES = te_format.c 

libtools.lib: $(libtools_lib_SOURCES) 
	$(srcdir)/../../engine/builder/te_build_win_lib $(srcdir) @host@ $@ \
        $(DESTDIR)/$(prefix) $(libtools_lib_SOURCES)
        
EXTRA_DIST=te_shell_cmd.c te_bufs.c te_log_fmt.c

else

lib_LIBRARIES = libtools.a

libtools_a_SOURCES = \
	te_bufs.c \
	te_shell_cmd.c \
	te_log_fmt.c \
	te_format.c

endif

install-exec-hook:
	touch -c -r libtools.* $(DESTDIR)/$(libdir)/libtools.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install
