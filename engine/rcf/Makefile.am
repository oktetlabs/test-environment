
AUTOMAKE_OPTIONS = foreign 

SUBDIRS=libltdl

INCLUDES = $(INCLTDL)

AM_CFLAGS=@TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include \
                      -I$(DESTDIR)/$(prefix)/../include \
                      @XML2_CFLAGS@
AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 

bin_PROGRAMS=rcf rcf_shutdown

rcf_SOURCES=rcf.c

rcf_shutdown_SOURCES = rcf_shutdown.c

rcf_shutdown_LDADD=-lrcfapi -lipc -llogger_ten -lpthread

rcf_LDFLAGS=-rdynamic -export-dynamic 

rcf_LDADD = $(LIBLTDL) @TE_LDADD@ -llogger_ten -lipcserver -lipc \
            -lltdl @XML2_LIBS@ -lpthread
rcf_DEPENDENCIES = $(LIBLTDL) @TE_DEPENDENCIES@ \
                   $(DESTDIR)/$(prefix)/lib/liblogger_ten.a \
                   $(DESTDIR)/$(prefix)/lib/libipcserver.a \
                   $(DESTDIR)/$(prefix)/lib/libipc.a

rcf_shutdown_DEPENDENCIES=$(DESTDIR)/$(prefix)/lib/librcfapi.a \
                          $(DESTDIR)/$(prefix)/lib/libipc.a 
