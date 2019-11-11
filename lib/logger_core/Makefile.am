
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) $(TE_CPPFLAGS_VAR)
AM_CFLAGS = @TE_CFLAGS@ $(TE_CFLAGS_VAR)
AM_LDFLAGS = @TE_LDFLAGS@ 


lib_LTLIBRARIES = liblogger_core.la

liblogger_core_la_SOURCES = logger_core.c


install-exec-hook:
	touch -c -r liblogger_core.la $(DESTDIR)/$(libdir)/liblogger_core.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
		touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install
