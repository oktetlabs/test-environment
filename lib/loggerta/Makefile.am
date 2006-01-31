
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@ 

include_HEADERS = logger_ta.h logger_ta_fast.h logger_ta_lock.h \
                  logger_ta_internal.h logfork.h


if WINDOWS

lib_LIBRARIES = libloggerta.lib

libloggerta_lib_SOURCES = logfork.c

EXTRA_DIST = logger_ta.c 

libloggerta.lib: $(libloggerta_lib_SOURCES) 
	$(srcdir)/../../engine/builder/te_build_win_lib $(srcdir) @host@ $@ \
        $(DESTDIR)/$(prefix) $(libloggerta_lib_SOURCES)

else

lib_LIBRARIES = libloggerta.a

libloggerta_a_SOURCES = logger_ta.c logfork.c

endif

install-exec-hook:
	touch -c -r libloggerta.* ${prefix}/lib/libloggerta.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install
