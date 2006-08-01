
SUBDIRS=rpc conf

ACLOCAL_AMFLAGS = -I $(TE_M4)

AM_CPPFLAGS = -I$(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/include $(TE_CPPFLAGS) \
              -include te_config.h -include config.h $(EXTRA_CPPFLAGS)

AM_CFLAGS = -fno-strict-aliasing $(TE_CFLAGS)

AM_LDFLAGS = -L$(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib -Wl,-Bstatic $(TE_LDFLAGS)

LIBS = $(TAD_LIBS) @LIBS@

taexecdir=$(prefix)/@with_name@

noinst_HEADERS = unix_internal.h 

taexec_PROGRAMS = ta

ta_SOURCES = main.c ftp_routines.c log_serial.c

ta_LDADD = symtbl.o conf/libconf.a rpc/libtarpc.a -lrcfpch \
           -l$(TAD) $(TE_LDADD) \
           -lrpcxdr -lrpc_types -lconf_oid -lloggerta -lrcfpch -ltools \
           $(EXTRA_LDADD)

CLEANFILES = symtbl.c

SYMTBL_OBJECTS = main.o ftp_routines.o log_serial.o 
SYMTBL_LIBS = $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libtools.a \
              $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/librcfpch.a \
              $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/lib$(TAD).a \
              conf/libconf.a rpc/libtarpc.a 

if ISCSI_TARGET_USE
SYMTBL_LIBS+=$(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libiscsi_unh_target.a
endif      

ta_DEPENDENCIES = @TE_DEPENDENCIES@ $(SYMTBL_LIBS) symtbl.o \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/librpcxdr.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/librpc_types.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libconf_oid.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libloggerta.a 

symtbl.c: $(SYMTBL_OBJECTS) $(SYMTBL_LIBS) @TE_DEPENDENCIES@
	$(srcdir)/../../engine/builder/te_generate_symtbl "${NM}" $@ $^ 
