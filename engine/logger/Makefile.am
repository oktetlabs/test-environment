
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@ @XML2_CFLAGS@
AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(libdir)


bin_PROGRAMS = te_logger te_log_shutdown te_log_flush te_tee

noinst_HEADERS = logger_internal.h


te_logger_SOURCES = logger.c logger_cnf.c logger_prc.c
te_logger_LDADD = @TE_LDADD@ -lipcserver -lrcfapi -lipc -ltools \
                  @XML2_LIBS@ -lpopt -lexpat -lpthread
te_logger_DEPENDENCIES = @TE_DEPENDENCIES@ \
                         $(DESTDIR)/$(libdir)/libipc.a \
                         $(DESTDIR)/$(libdir)/libipcserver.a \
                         $(DESTDIR)/$(libdir)/librcfapi.a \
                         $(DESTDIR)/$(libdir)/libtools.a 
                       
te_log_shutdown_SOURCES = te_log_shutdown.c
te_log_shutdown_LDADD = @TE_LDADD@ -lipc -lpthread
te_log_shutdown_DEPENDENCIES = @TE_DEPENDENCIES@ \
                               $(DESTDIR)/$(libdir)/libipc.a \
                               $(DESTDIR)/$(libdir)/liblogger_ten.a

te_log_flush_SOURCES = te_log_flush.c
te_log_flush_LDADD = @TE_LDADD@ -lrcfapi -llogger_ten -lipc -ltools \
                     -lpthread -lexpat 
te_log_flush_DEPENDENCIES = @TE_DEPENDENCIES@ \
                            $(DESTDIR)/$(libdir)/libipc.a \
                            $(DESTDIR)/$(libdir)/liblogger_ten.a \
                            $(DESTDIR)/$(libdir)/librcfapi.a \
                            $(DESTDIR)/$(libdir)/libtools.a

te_tee_SOURCES = te_tee.c
te_tee_LDADD = -llogger_ten -lipc -ltools @TE_LDADD@ -lpthread 
te_tee_DEPENDENCIES = @TE_DEPENDENCIES@ \
                      $(DESTDIR)/$(libdir)/liblogger_ten.a \
                      $(DESTDIR)/$(libdir)/libipc.a \
                      $(DESTDIR)/$(libdir)/libtools.a


mybindir=$(prefix)/bin
dist_mybin_SCRIPTS = te_log_archive te_log_init te_log_message

