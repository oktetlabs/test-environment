
AM_CPPFLAGS = -I$(DESTDIR)/$(prefix)/../@host@/include \
              -I$(DESTDIR)/$(prefix)/../include

AM_CFLAGS = 

AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/../@host@/lib


bin_PROGRAMS = taswitch-ctl

taswitch_ctl_LDADD = symtbl.o @TE_LDADD@ -lpthread -lstdc++

taswitch_ctl_SOURCES = main.c conf.c

taswitch_ctl_DEPENDENCIES = @TE_DEPENDENCIES@ symtbl.o


symtbl.o: main.o conf.o @TE_DEPENDENCIES@
	te_generate_symtbl ${NM} symtbl.c $^ 
	${CC} -c -o $@ symtbl.c
