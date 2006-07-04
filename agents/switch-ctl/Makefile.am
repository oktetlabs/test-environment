
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/include 
AM_CFLAGS = @TE_CFLAGS@ 
AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib


bin_PROGRAMS = taswitch-ctl

#
# -lrcfpch must be before @TE_LDADD@ since the last may contain
# -ltad_ch with names used in -lrcfpch.
#
taswitch_ctl_LDADD = symtbl.o -lrcfpch -l@TAD@ @TE_LDADD@ -lconf_oid \
                     -lcomm_net_agent -lloggerta -ltools -lpthread -lstdc++

taswitch_ctl_SOURCES = main.c conf.c

taswitch_ctl_DEPENDENCIES = @TE_DEPENDENCIES@ symtbl.o \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/librcfpch.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/lib@TAD@.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libconf_oid.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libcomm_net_agent.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libloggerta.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libtools.a


symtbl.o: main.o conf.o @TE_DEPENDENCIES@
	te_generate_symtbl ${NM} symtbl.c $^ 
	${CC} -c -o $@ symtbl.c
