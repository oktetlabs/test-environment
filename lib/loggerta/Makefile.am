
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@ 

include_HEADERS = logger_ta.h logger_ta_fast.h logger_ta_lock.h \
                  logger_ta_internal.h logfork.h


lib_LIBRARIES = libloggerta.a

if WINDOWS

WIN_SRC = logfork.c

EXTRA_DIST = logger_ta.c 

libloggerta.a: $(WIN_SRC) 
	TE_INSTALL=$(DESTDIR)/$(prefix)/.. \
        $(srcdir)/../../engine/builder/te_build_win_lib $(srcdir) @host@ $@ \
        $(DESTDIR)/$(prefix) $(WIN_SRC)

install-exec-hook:
	if test -e $(lib_LIBRARIES) -a ! -s $(lib_LIBRARIES) \
                -a -n "$$TE_WIN32_BUILD_HOST" ; then \
            rm $(lib_LIBRARIES) ; $(MAKE) install ; \
        fi

else

libloggerta_a_SOURCES = logger_ta.c logfork.c

install-exec-hook:
	touch -c -r libloggerta.* $(DESTDIR)/$(libdir)/libloggerta.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install

endif

