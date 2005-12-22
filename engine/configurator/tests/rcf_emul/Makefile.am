
AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@ @XML2_CFLAGS@ 
AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(libdir) -L./../db

noinst_HEADERS = rcf_emul.h

noinst_LTLIBRARIES = librcf_emul.la

librcf_emul_la_SOURCES = rcf_emul.c

librcf_emul_la_LIBADD = $(DESTDIR)/$(libdir) -L$./../db/.libs \
                        -llogger_ten -lrcfapi -ldb -lipcserver -lipc

librcf_emul_la_DEPENDENCIES = @TE_DEPENDENCIES@ \
                              $(DESTDIR)/$(libdir)/liblogger_ten.a \
                              $(DESTDIR)/$(libdir)/librcfapi.a \
                              $(DESTDIR)/$(libdir)/libipcserver.a \
                              $(DESTDIR)/$(libdir)/libipc.a \
                              ./../db/.libs/libdb.a
      
all: install
