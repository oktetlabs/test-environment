
AUTOMAKE_OPTIONS = foreign 

AM_CPPFLAGS = -I$(DESTDIR)/$(prefix)/include \
              -I$(DESTDIR)/$(prefix)/../include

AM_CFLAGS = @XML2_CFLAGS@

AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 


bin_PROGRAMS = te_rcf te_rcf_shutdown


te_rcf_SOURCES = rcf.c

te_rcf_LDFLAGS = -rdynamic -export-dynamic 

te_rcf_LDADD = @TE_LDADD@ -llogger_ten -lipcserver -lipc \
               @XML2_LIBS@ -ldl -lpthread

te_rcf_DEPENDENCIES = @TE_DEPENDENCIES@ \
                      $(DESTDIR)/$(prefix)/lib/liblogger_ten.a \
                      $(DESTDIR)/$(prefix)/lib/libipcserver.a \
                      $(DESTDIR)/$(prefix)/lib/libipc.a


te_rcf_shutdown_SOURCES = rcf_shutdown.c

te_rcf_shutdown_LDADD = -lrcfapi -llogger_ten -lipc -lpthread

te_rcf_shutdown_DEPENDENCIES = $(DESTDIR)/$(prefix)/lib/librcfapi.a \
                               $(DESTDIR)/$(prefix)/lib/libipc.a

