
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) $(TE_CPPFLAGS_VAR)
AM_CFLAGS = @TE_CFLAGS@ $(TE_CFLAGS_VAR)

include_HEADERS=log_msg_view.h log_msg_filter.h

lib_LTLIBRARIES=liblog_proc.la

liblog_proc_la_SOURCES=log_msg_view.c log_msg_filter.c

liblog_proc_la_LIBADD=-ltools

install-exec-hook:
	touch -c -r liblog_proc.la $(DESTDIR)/$(libdir)/liblog_proc.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
		touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install
