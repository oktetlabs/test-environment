
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) $(TE_CPPFLAGS_VAR)

AM_CFLAGS = @TE_CFLAGS@ $(TE_CFLAGS_VAR) @JANSSON_CFLAGS@

AM_LDFLAGS = -L$(DESTDIR)/$(libdir) @JANSSON_LIBS@

lib_LTLIBRARIES=libtapi_performance.la

include_HEADERS= \
        tapi_performance.h \
        tapi_iperf3.h

libtapi_performance_la_SOURCES= \
        tapi_performance.c \
        tapi_iperf3.c


install-exec-hook:
	touch -c -r libtapi_performance.la $(DESTDIR)/$(libdir)/libtapi_performance.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
		touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install
