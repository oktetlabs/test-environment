
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) $(TE_CPPFLAGS_VAR)

AM_CFLAGS = @TE_CFLAGS@ $(TE_CFLAGS_VAR)

AM_LDFLAGS = -L$(DESTDIR)/$(libdir)

lib_LTLIBRARIES=libtapi_gtest.la

include_HEADERS= \
    tapi_gtest.h

libtapi_gtest_la_SOURCES= \
    tapi_gtest.c

install-exec-hook:
	touch -c -r libtapi_gtest.la $(DESTDIR)/$(libdir)/libtapi_gtest.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
		touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install
