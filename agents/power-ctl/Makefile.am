ACLOCAL_AMFLAGS = -I $(TE_M4)

AM_CPPFLAGS = -I$(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/include $(TE_CPPFLAGS) \
              -include te_config.h -include config.h
AM_CFLAGS = $(TE_CFLAGS)

AM_LDFLAGS = -L$(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib -Wl,-Bstatic \
             @TE_LDFLAGS@

LIBS = -Wl,-Bdynamic @TAD_LIBS@ @LIBS@

noinst_HEADERS = ta_snmp.h

taexecdir = $(prefix)/@with_name@
taexec_PROGRAMS = ta

ta_SOURCES = main.c ta_snmp.c

ta_LDADD = -lrcfpch -l@TAD@ @TE_LDADD@ -lrpcxdr -lconf_oid \
           -lloggerta -ltools

ta_DEPENDENCIES = @TE_DEPENDENCIES@ \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/librcfpch.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/lib@TAD@.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libconf_oid.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libloggerta.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libtools.a

