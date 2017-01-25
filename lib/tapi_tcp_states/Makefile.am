
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) $(TE_CPPFLAGS_VAR)
AM_CFLAGS = @TE_CFLAGS@ $(TE_CFLAGS_VAR)
AM_LDFLAGS = @TE_LDFLAGS@

include_HEADERS = \
		  tapi_tcp_states.h

noinst_HEADERS = \
		 tapi_tcp_states_internal.h

lib_LTLIBRARIES = libtapi_tcp_states.la

libtapi_tcp_states_la_SOURCES = \
			   tapi_tcp_states.c \
			   tapi_tcp_states_sock.c \
			   tapi_tcp_states_csap.c


install-exec-hook:
	touch -c -r libtapi_tcp_states.la $(DESTDIR)/$(libdir)/libtapi_tcp_states.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
		touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install
