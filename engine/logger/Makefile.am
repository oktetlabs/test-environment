
AM_CPPFLAGS = -I$(DESTDIR)/$(prefix)/include \
              -I$(DESTDIR)/$(prefix)/../include

# TE_CFLAGS are already included in CFLAGS in configure.ac
AM_CFLAGS = @XML2_CFLAGS@

AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 


bin_PROGRAMS=te_logger te_log_shutdown te_log_flush

noinst_HEADERS=logger_prc_internal.h logger_internal.h

te_logger_SOURCES=logger.c logger_cnf.c logger_prc.c

te_log_shutdown_SOURCES = te_log_shutdown.c

te_log_flush_SOURCES = te_log_flush.c

te_logger_LDADD = @TE_LDADD@ -lipcserver -lrcfapi -lipc -llogger_ten -lpthread @XML2_LIBS@
te_log_shutdown_LDADD = @TE_LDADD@ -lipc -llogger_ten

te_log_flush_LDADD = @TE_LDADD@ -lrcfapi -lipc -llogger_ten -lpthread

te_logger_DEPENDENCIES = @TE_DEPENDENCIES@ \
                         $(DESTDIR)/$(prefix)/lib/libipc.a \
                         $(DESTDIR)/$(prefix)/lib/libipcserver.a \
                         $(DESTDIR)/$(prefix)/lib/liblogger_ten.a \
                         $(DESTDIR)/$(prefix)/lib/librcfapi.a
                       
te_log_shutdown_DEPENDENCIES = @TE_DEPENDENCIES@ \
                               $(DESTDIR)/$(prefix)/lib/libipc.a \
                               $(DESTDIR)/$(prefix)/lib/liblogger_ten.a

te_log_flush_DEPENDENCIES = @TE_DEPENDENCIES@ \
                            $(DESTDIR)/$(prefix)/lib/libipc.a \
                            $(DESTDIR)/$(prefix)/lib/liblogger_ten.a \
                            $(DESTDIR)/$(prefix)/lib/librcfapi.a
                            

mybindir=$(prefix)/bin

mybin_SCRIPTS=te_log_archive te_log_full_name te_log_init te_log_message

EXTRA_DIST=te_log_archive te_log_full_name te_log_init te_log_message 
