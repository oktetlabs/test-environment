
AM_CPPFLAGS = -I$(DESTDIR)/$(prefix)/../@host@/include 
AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/../@host@/lib


bin_PROGRAMS = taswitch-ctl

#
# -lrcfpch must be before @TE_LDADD@ since the last may contain
# -ltad_ch with names used in -lrcfpch.
#
taswitch_ctl_LDADD = symtbl.o -lrcfpch -l@TAD@ @TE_LDADD@ -lconf_oid \
                     -lcomm_net_agent -lloggerta -ltools -lpthread -lstdc++

taswitch_ctl_SOURCES = main.c conf.c

taswitch_ctl_DEPENDENCIES = @TE_DEPENDENCIES@ symtbl.o \
    $(DESTDIR)/$(prefix)/../@host@/lib/librcfpch.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/lib@TAD@.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/libconf_oid.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/libcomm_net_agent.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/libloggerta.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/libtools.a


symtbl.o: main.o conf.o @TE_DEPENDENCIES@
	te_generate_symtbl ${NM} symtbl.c $^ 
	${CC} -c -o $@ symtbl.c
