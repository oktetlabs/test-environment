
AM_CPPFLAGS = @TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include 
AM_CFLAGS = @XML2_CFLAGS@
AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 


bin_PROGRAMS = te_rcf te_rcf_shutdown te_rcf_tadead


te_rcf_SOURCES = rcf.c

te_rcf_LDFLAGS = -rdynamic -export-dynamic 

te_rcf_LDADD = @TE_LDADD@ -lipcserver -llogger_ten -lipc -ltools \
               @XML2_LIBS@ -ldl -lexpat -lpopt -lpthread 

te_rcf_DEPENDENCIES = @TE_DEPENDENCIES@ \
                      $(DESTDIR)/$(prefix)/lib/liblogger_ten.a \
                      $(DESTDIR)/$(prefix)/lib/libipcserver.a \
                      $(DESTDIR)/$(prefix)/lib/libipc.a \
                      $(DESTDIR)/$(prefix)/lib/libtools.a

te_rcf_shutdown_SOURCES = rcf_shutdown.c

te_rcf_shutdown_LDADD = -llogger_ten -lipc -ltools -lpthread

te_rcf_shutdown_DEPENDENCIES = $(DESTDIR)/$(prefix)/lib/liblogger_ten.a \
                               $(DESTDIR)/$(prefix)/lib/libipc.a \
                               $(DESTDIR)/$(prefix)/lib/libtools.a


te_rcf_tadead_SOURCES = rcf_tadead.c

# Use static linkage for tool to be used by human
te_rcf_tadead_LDFLAGS = -static

te_rcf_tadead_LDADD = -llogger_ten -lipc -ltools -lpthread

te_rcf_tadead_DEPENDENCIES = $(DESTDIR)/$(prefix)/lib/liblogger_ten.a \
                             $(DESTDIR)/$(prefix)/lib/libipc.a \
                             $(DESTDIR)/$(prefix)/lib/libtools.a

