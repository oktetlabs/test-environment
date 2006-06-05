
SUBDIRS=rpc conf

ACLOCAL_AMFLAGS = -I $(TE_M4)

AM_CPPFLAGS = -I$(DESTDIR)/$(prefix)/../@host@/include $(TE_CPPFLAGS) \
              -include te_config.h -include config.h $(EXTRA_CPPFLAGS)

AM_CFLAGS = -fno-strict-aliasing $(TE_CFLAGS)

AM_LDFLAGS = -L$(DESTDIR)/$(prefix)/../@host@/lib -Wl,-Bstatic @TE_LDFLAGS@

LIBS = -Wl,-Bdynamic @TAD_LIBS@ @LIBS@

tadir=$(prefix)/@with_name@

noinst_HEADERS = unix_internal.h 

ta_PROGRAMS = ta

ta_SOURCES = main.c ftp_routines.c log_serial.c

ta_LDADD = symtbl.o conf/libconf.a rpc/libtarpc.a -lrcfpch \
           -l@TAD@ @TE_LDADD@ \
           -lrpcxdr -lrpc_types -lconf_oid -lloggerta -lrcfpch -ltools \
           $(EXTRA_LDADD)

CLEANFILES = symtbl.c

SYMTBL_OBJECTS = main.o ftp_routines.o log_serial.o 
SYMTBL_LIBS = $(DESTDIR)/$(prefix)/../@host@/lib/libtools.a \
              $(DESTDIR)/$(prefix)/../@host@/lib/librcfpch.a \
              $(DESTDIR)/$(prefix)/../@host@/lib/lib@TAD@.a \
              conf/libconf.a rpc/libtarpc.a

ta_DEPENDENCIES = @TE_DEPENDENCIES@ $(SYMTBL_LIBS) symtbl.o \
    $(DESTDIR)/$(prefix)/../@host@/lib/librpcxdr.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/libconf_oid.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/libloggerta.a 

symtbl.c: $(SYMTBL_OBJECTS) $(SYMTBL_LIBS) @TE_DEPENDENCIES@
	$(srcdir)/../../engine/builder/te_generate_symtbl "${NM}" $@ $^ 

install-data:
