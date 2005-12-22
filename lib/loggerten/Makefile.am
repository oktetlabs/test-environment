
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@ 
AM_LDFLAGS = @TE_LDFLAGS@ 


include_HEADERS = logger_ten.h logger_ten_int.h


lib_LTLIBRARIES = liblogger_ten.la

liblogger_ten_la_SOURCES = logger_ten.c


install-exec-hook:
	touch -c -r .libs/liblogger_ten.la ${prefix}/lib/liblogger_ten.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
		touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install
