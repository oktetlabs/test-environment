
AM_CPPFLAGS = -I$(DESTDIR)/$(prefix)/include
AM_LDFLAGS = @TE_LDFLAGS@ 
AM_YFLAGS = -d


include_HEADERS = \
	tapi_env.h

lib_LTLIBRARIES = libtapi.la

libtapi_la_SOURCES = \
	tapi_env.c \
	env_gram.y \
	env_lex.l


BUILT_SOURCES += env_gram.h


install-exec-hook:
	touch -c -r .libs/libtapi.la $(DESTDIR)/$(prefix)/lib/libtapi.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install
