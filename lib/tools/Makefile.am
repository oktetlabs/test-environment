
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) $(TE_CPPFLAGS_VAR)

AM_CFLAGS = @TE_CFLAGS@ -fno-strict-aliasing $(TE_CFLAGS_VAR)

include_HEADERS = \
	te_serial_parser.h \
	te_tools.h \
	te_sleep.h \
	te_bufs.h \
	te_alloc.h \
	te_sockaddr.h \
	te_shell_cmd.h \
	te_log_fmt.h \
	logger_file.h \
	te_kernel_log.h \
	te_format.h \
	te_iscsi.h \
	tq_string.h \
	te_string.h \
	log_bufs.h \
	te_sigmap.h

if WINDOWS

lib_LIBRARIES = libtools.a

libtools_a_CFLAGS = -DTE_DUMMY
libtools_a_SOURCES = te_bufs.c te_alloc.c te_sockaddr.c te_format.c

libtools.a: $(libtools_a_SOURCES) 
	TE_INSTALL=$(DESTDIR)/$(prefix)/.. \
        PATH="$${PATH}:$(TE_BASE)/engine/builder:$(TE_PATH)" \
        te_build_win_lib $(TE_PLATFORM) $@ $(DESTDIR)/$(prefix) $^
        
install-exec-hook:
	if test -e $(lib_LIBRARIES) -a ! -s $(lib_LIBRARIES) \
                -a -n "$$TE_WIN32_BUILD_HOST" ; then \
            rm $(lib_LIBRARIES) ; $(MAKE) install ; \
        fi

EXTRA_DIST = te_shell_cmd.c te_log_fmt.c te_log_file.c log_bufs.c \
             tq_string.c te_string.c logger_file.c te_log_kernel.c

else

lib_LTLIBRARIES = libtools.la liblog_bufs.la liblogger_file.la

libtools_la_SOURCES = \
	te_bufs.c \
	te_alloc.c \
	te_sockaddr.c \
	te_shell_cmd.c \
	te_log_fmt.c \
	te_log_file.c \
	te_kernel_log.c \
	te_format.c \
	tq_string.c \
	te_string.c \
	te_sigmap.c

liblog_bufs_la_SOURCES = log_bufs.c

liblogger_file_la_SOURCES = logger_file.c

install-exec-hook:
	for i in $(lib_LTLIBRARIES) ; do \
		j=`echo $${i} | sed -e 's/.la//'`; \
		touch -c -r $$i $(DESTDIR)/$(libdir)/$${j}.* ; \
	done

install-data-hook:
	for i in $(include_HEADERS) ; do \
		touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install

endif

