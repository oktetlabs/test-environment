
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@ 
AM_LDFLAGS = @TE_LDFLAGS@ 
AM_YFLAGS = -d


include_HEADERS = \
	tapi_env.h

lib_LTLIBRARIES = libtapi_env.la

libtapi_env_la_SOURCES = \
	tapi_env.c \
	env_gram.y \
	env_lex.l


BUILT_SOURCES = env_gram.h


install-exec-hook:
	touch -c -r .libs/libtapi_env.la $(DESTDIR)/$(libdir)/libtapi_env.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install
