
AM_CPPFLAGS = @TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include \
              -fno-strict-aliasing
AM_CFLAGS = @XML2_CFLAGS@
AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 

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
              -llogger_ten -lipcserver -lipc @XML2_LIBS@ -lpthread \
              -lpopt -lexpat

te_cs_shutdown_LDADD = @TE_LDADD@ -llogger_ten -lipc -lpthread

te_cs_DEPENDENCIES = @TE_DEPENDENCIES@ \
                     $(DESTDIR)/$(prefix)/lib/librcfapi.a \
                     $(DESTDIR)/$(prefix)/lib/libconf_types.a \
                     $(DESTDIR)/$(prefix)/lib/libconf_oid.a \
                     $(DESTDIR)/$(prefix)/lib/liblogger_ten.a \
                     $(DESTDIR)/$(prefix)/lib/libipcserver.a \
                     $(DESTDIR)/$(prefix)/lib/libipc.a

te_cs_shutdown_DEPENDENCIES = @TE_DEPENDENCIES@ \
                              $(DESTDIR)/$(prefix)/lib/liblogger_ten.a \
                              $(DESTDIR)/$(prefix)/lib/libipc.a


