 
AUTOMAKE_OPTIONS = foreign 

AM_CFLAGS=@TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/../@host@/include \
                      -I$(DESTDIR)/$(prefix)/../include

taisosctl_SOURCES=isosctl.c

AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/../@host@/lib

taisosctl_LDADD=-lrcfpch @TE_LDADD@ -lconf_oid -lcomm_net_agent \
                -lloggerta -lpthread

taisosctl_DEPENDENCIES=@TE_DEPENDENCIES@ 

bin_PROGRAMS=taisosctl
