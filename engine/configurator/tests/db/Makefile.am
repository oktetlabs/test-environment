AM_CPPFLAGS = @TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include

AM_CFLAGS = @XML2_CFLAGS@
AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib

noinst_HEADERS = db.h

noinst_LTLIBRARIES = libdb.la

libdb_la_SOURCES = db.c

libdb_la_LIBADD = -L$(DESTDIR)/$(prefix)/lib -llogger_ten

libdb_la_DEPENDENCIES = @TE_DEPENDENCIES@ \
                        $(DESTDIR)/$(prefix)/lib/liblogger_ten.a

all: install
