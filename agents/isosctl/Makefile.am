 
AUTOMAKE_OPTIONS = foreign 

AM_CFLAGS=@TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/../@host@/include

taisosctl_SOURCES=isosctl.c

AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/../@host@/lib

taisosctl_LDADD=-lrcfpch -l@TAD@ @TE_LDADD@ -lconf_oid -lcomm_net_agent \
                -lloggerta -lpthread

taisosctl_DEPENDENCIES=@TE_DEPENDENCIES@ \
    $(DESTDIR)/$(prefix)/../@host@/lib/librcfpch.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/lib@TAD@.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/libconf_oid.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/libcomm_net_agent.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/libloggerta.a

bin_PROGRAMS=taisosctl
