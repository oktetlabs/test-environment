 
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ \
              -I$(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/include \
              $(TE_CPPFLAGS_VAR)
AM_CFLAGS = @TE_CFLAGS@ $(TE_CFLAGS_VAR)

taisosctl_SOURCES=isosctl.c

AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib

taisosctl_LDADD=-lrcfpch -l@TAD@ @TE_LDADD@ -lconf_oid -lcomm_net_agent \
                -lloggerta -ltools -lpthread

taisosctl_DEPENDENCIES=@TE_DEPENDENCIES@ \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/librcfpch.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/lib@TAD@.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libconf_oid.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libcomm_net_agent.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libloggerta.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libtools.a

bin_PROGRAMS=taisosctl
