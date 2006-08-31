
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@

include_HEADERS = \
	te_tools.h \
	te_sleep.h \
	te_bufs.h \
	te_alloc.h \
	te_sockaddr.h \
	te_shell_cmd.h \
	te_log_fmt.h \
	te_format.h \
	te_iscsi.h \
	tq_string.h \
	log_bufs.h

if WINDOWS

lib_LIBRARIES = libtools.a

libtools_a_CFLAGS = -DTE_DUMMY
WIN_SRC = te_bufs.c te_alloc.c te_sockaddr.c te_format.c
libtools_a_SOURCES=$(WIN_SRC)

libtools.a: $(WIN_SRC) 
	TE_INSTALL=$(DESTDIR)/$(prefix)/.. \
        $(srcdir)/../../engine/builder/te_build_win_lib $(srcdir) \
        $(TE_PLATFORM) $@ $(DESTDIR)/$(prefix) $(WIN_SRC)
        
install-exec-hook:
	if test -e $(lib_LIBRARIES) -a ! -s $(lib_LIBRARIES) \
                -a -n "$$TE_WIN32_BUILD_HOST" ; then \
            rm $(lib_LIBRARIES) ; $(MAKE) install ; \
        fi

EXTRA_DIST=te_shell_cmd.c te_log_fmt.c log_bufs.c

else

lib_LTLIBRARIES = libtools.la liblog_bufs.la

libtools_la_SOURCES = \
	te_bufs.c \
	te_alloc.c \
	te_sockaddr.c \
	te_shell_cmd.c \
	te_log_fmt.c \
	te_format.c \
	tq_string.c

liblog_bufs_la_SOURCES = log_bufs.c

install-exec-hook:
	touch -c -r libtools.la $(DESTDIR)/$(libdir)/libtools.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
		touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install

endif

