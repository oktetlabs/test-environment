
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ $(TIRPC_CFLAGS) \
              -I$(DESTDIR)/$(includedir) $(TE_CPPFLAGS_VAR)

AM_CFLAGS = @TE_CFLAGS@ $(TE_CFLAGS_VAR)

AM_LDFLAGS = -L$(DESTDIR)/$(libdir)

include_HEADERS= \
	tapi_serial.h

lib_LTLIBRARIES = libtapi_serial.la

libtapi_serial_la_SOURCES = \
	rpc_serial.c \
	tapi_serial.c

libtapi_serial_la_LIBADD = $(TIRPC_LIBS)

install-exec-hook:
	for i in ${lib_LTLIBRARIES} ; do \
	    touch -c -r $$i $(DESTDIR)/$(prefix)/lib/$$i ; \
	done

install-data-hook:
	for i in $(include_HEADERS) ; do \
		touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install
