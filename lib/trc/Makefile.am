
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) $(TE_CPPFLAGS_VAR)
AM_CFLAGS = @TE_CFLAGS@ @XML2_CFLAGS@ $(TE_CFLAGS_VAR)

include_HEADERS = \
	te_trc.h \
	trc_db.h

lib_LTLIBRARIES = libtrc.la

libtrc_la_SOURCES = \
	compare.c \
	db.c \
	db_io.c \
	db_walker.c

install-exec-hook:
	touch -c -r libtrc.la $(DESTDIR)/$(libdir)/libtrc.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
		touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install
