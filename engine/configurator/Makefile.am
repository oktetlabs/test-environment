
AM_CPPFLAGS = -I$(DESTDIR)/$(prefix)/include \
              -I$(DESTDIR)/$(prefix)/../include 

# TE_CFLAGS are already included in CFLAGS in configure.ac
AM_CFLAGS = @XML2_CFLAGS@

AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 


bin_PROGRAMS = configurator conf_shutdown

noinst_HEADERS = conf_db.h conf_dh.h conf_backup.h conf_ta.h conf_defs.h

configurator_SOURCES = conf_db.c conf_dh.c conf_main.c conf_backup.c conf_ta.c 

conf_shutdown_SOURCES = conf_shutdown.c

configurator_LDADD = @TE_LDADD@ -lipcserver -lrcfapi  -lconf_types -lconf_oid -lipc -llogger_ten -lxml2 -lpthread @XML2_LIBS@

conf_shutdown_LDADD = @TE_LDADD@ -llogger_ten -lipc -lpthread

configurator_DEPENDENCIES = @TE_DEPENDENCIES@ \
                            $(DESTDIR)/$(prefix)/lib/libipc.a \
                            $(DESTDIR)/$(prefix)/lib/libipcserver.a \
                            $(DESTDIR)/$(prefix)/lib/liblogger_ten.a \
                            $(DESTDIR)/$(prefix)/lib/libconf_types.a \
                            $(DESTDIR)/$(prefix)/lib/libconf_oid.a \
                            $(DESTDIR)/$(prefix)/lib/librcfapi.a

conf_shutdown_DEPENDENCIES = @TE_DEPENDENCIES@ \
                             $(DESTDIR)/$(prefix)/lib/libipc.a 

