
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) $(TE_CPPFLAGS_VAR)
AM_CFLAGS = @TE_CFLAGS@ @XML2_CFLAGS@ $(TE_CFLAGS_VAR)
AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(libdir)


bin_PROGRAMS = te_logger te_log_shutdown te_log_flush te_tee te_log_get_path

noinst_HEADERS = logger_internal.h te_log_sniffers.h

te_log_get_path_SOURCES = te_log_get_path.c logger_cnf_int.c
te_log_get_path_LDADD = @XML2_LIBS@ -lyaml

te_logger_SOURCES = logger.c logger_cnf.c logger_cnf_int.c logger_prc.c \
                    te_log_sniffers.c
te_logger_LDADD = @TE_LDADD@ -lipcserver -lrcfapi -lipc -ltools -llogger_core \
                  @XML2_LIBS@ -lpopt -lexpat -lyaml -lpthread
te_logger_DEPENDENCIES = @TE_DEPENDENCIES@ \
                         $(DESTDIR)/$(libdir)/liblogger_core.a \
                         $(DESTDIR)/$(libdir)/libipc.a \
                         $(DESTDIR)/$(libdir)/libipcserver.a \
                         $(DESTDIR)/$(libdir)/librcfapi.a \
                         $(DESTDIR)/$(libdir)/libtools.a 
                       
te_log_shutdown_SOURCES = te_log_shutdown.c
te_log_shutdown_LDADD = @TE_LDADD@ -lipc -lpthread
te_log_shutdown_DEPENDENCIES = @TE_DEPENDENCIES@ \
                               $(DESTDIR)/$(libdir)/libipc.a

te_log_flush_SOURCES = te_log_flush.c
te_log_flush_LDADD = @TE_LDADD@ -lrcfapi -llogger_ten -lipc -ltools -llogger_core \
                     -lpthread -lexpat 
te_log_flush_DEPENDENCIES = @TE_DEPENDENCIES@ \
                            $(DESTDIR)/$(libdir)/libipc.a \
                            $(DESTDIR)/$(libdir)/liblogger_core.a \
                            $(DESTDIR)/$(libdir)/liblogger_ten.a \
                            $(DESTDIR)/$(libdir)/librcfapi.a \
                            $(DESTDIR)/$(libdir)/libtools.a

te_tee_SOURCES = te_tee.c
te_tee_LDADD = -llogger_ten -lipc -ltools -llogger_core @TE_LDADD@ -lpthread
te_tee_DEPENDENCIES = @TE_DEPENDENCIES@ \
                      $(DESTDIR)/$(libdir)/liblogger_core.a \
                      $(DESTDIR)/$(libdir)/liblogger_ten.a \
                      $(DESTDIR)/$(libdir)/libipc.a \
                      $(DESTDIR)/$(libdir)/libtools.a


mybindir=$(prefix)/bin
dist_mybin_SCRIPTS = te_log_archive te_log_init te_log_message

