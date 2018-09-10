
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) $(TE_CPPFLAGS_VAR)

AM_CFLAGS = @TE_CFLAGS@ $(TE_CFLAGS_VAR) @JANSSON_CFLAGS@

AM_LDFLAGS = -L$(DESTDIR)/$(libdir) @JANSSON_LIBS@

lib_LTLIBRARIES=libtapi_fio.la

noinst_HEADERS= \
    fio.h \
    fio_internal.h

include_HEADERS= \
    tapi_fio.h

libtapi_fio_la_SOURCES= \
    tapi_fio.c \
    fio.c \
    fio_internal.c


install-exec-hook:
	touch -c -r libtapi_fio.la $(DESTDIR)/$(libdir)/libtapi_fio.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
		touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install
