
AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@ @XML2_CFLAGS@
AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(libdir)

noinst_HEADERS = db.h

noinst_LTLIBRARIES = libdb.la

libdb_la_SOURCES = db.c

libdb_la_LIBADD = -L$(DESTDIR)/$(libdir) -llogger_ten

libdb_la_DEPENDENCIES = @TE_DEPENDENCIES@ \
                        $(DESTDIR)/$(libdir)/liblogger_ten.a

all: install
