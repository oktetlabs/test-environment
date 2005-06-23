AM_CPPFLAGS = @TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include 
    
AM_CFLAGS = @XML2_CFLAGS@ 
AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib -L./../db

noinst_HEADERS = rcf_emul.h

noinst_LTLIBRARIES = librcf_emul.la

librcf_emul_la_SOURCES = rcf_emul.c

librcf_emul_la_LIBADD = $(DESTDIR)/$(prefix)/lib -L$./../db/.libs -llogger_ten -lrcfapi -lrpcxdr -ldb -lipcserver -lipc

librcf_emul_la_DEPENDENCIES = @TE_DEPENDENCIES@ \
                              $(DESTDIR)/$(prefix)/lib/liblogger_ten.a \
                              $(DESTDIR)/$(prefix)/lib/librcfapi.a \
                              $(DESTDIR)/$(prefix)/lib/librpcxdr.a \
                              $(DESTDIR)/$(prefix)/lib/libipcserver.a \
                              $(DESTDIR)/$(prefix)/lib/libipc.a \
                              ./../db/.libs/libdb.a
      
all: install
