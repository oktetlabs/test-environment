
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) -fno-strict-aliasing
AM_CFLAGS = @TE_CFLAGS@ @XML2_CFLAGS@
AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(libdir)

#if CONFIGURATOR_DEBUG
#SUBDIRS = . tests
#else
#EXTRA_DIST = tests
#endif

bin_PROGRAMS = te_cs te_cs_shutdown 

noinst_HEADERS = conf_db.h conf_dh.h conf_backup.h conf_ta.h conf_defs.h

te_cs_SOURCES = conf_db.c conf_dh.c conf_main.c conf_backup.c conf_ta.c 
 
te_cs_shutdown_SOURCES = conf_shutdown.c

te_cs_LDADD = @TE_LDADD@ -lrcfapi -lconf_types -lconf_oid \
              -llogger_ten -lipcserver -lipc -ltools @XML2_LIBS@ \
              -lpthread -lpopt -lexpat 

te_cs_shutdown_LDADD = @TE_LDADD@ -llogger_ten -lipc -ltools -lpthread 

te_cs_DEPENDENCIES = @TE_DEPENDENCIES@ \
                     $(DESTDIR)/$(libdir)/librcfapi.a \
                     $(DESTDIR)/$(libdir)/libconf_types.a \
                     $(DESTDIR)/$(libdir)/libconf_oid.a \
                     $(DESTDIR)/$(libdir)/liblogger_ten.a \
                     $(DESTDIR)/$(libdir)/libipcserver.a \
                     $(DESTDIR)/$(libdir)/libipc.a \
                     $(DESTDIR)/$(libdir)/libtools.a

te_cs_shutdown_DEPENDENCIES = @TE_DEPENDENCIES@ \
                              $(DESTDIR)/$(libdir)/liblogger_ten.a \
                              $(DESTDIR)/$(libdir)/libipc.a \
                              $(DESTDIR)/$(libdir)/libtools.a
