
SUBDIRS=rpc conf

ACLOCAL_AMFLAGS = -I $(TE_M4)

AM_CPPFLAGS = -I$(DESTDIR)/$(prefix)/../@host@/include $(TE_CPPFLAGS) \
              -include te_config.h -include config.h @EXTRA_CPPFLAGS@

AM_CFLAGS = -fno-strict-aliasing $(TE_CFLAGS)

#
# -lrcfpch must be before -l@TAD@ since rcfpch contains references to
# functions provided by -l@TAD@. @TE_LDADD@ must be after -l@TAD@,
# since @TE_LDADD@ may contain ASN and NDN libraries.
#
TA_LDADD = symtbl.o conf/libconf.a rpc/libtarpc.a -lrcfpch -l@TAD@ @TE_LDADD@ \
           -lrpcxdr -lrpc_types -lconf_oid -lloggerta -lrcfpch -ltools

noinst_HEADERS = \
	unix_internal.h 

TA_SOURCES = \
	main.c \
	ftp_routines.c \
	log_serial.c

TA_SYMTBL_OBJECTS = main.o ftp_routines.o log_serial.o 

CLEANFILES = symtbl.c

LIBS = -Wl,-Bdynamic @TAD_LIBS@ @LIBS@

AM_LDFLAGS = -L$(DESTDIR)/$(prefix)/../@host@/lib -Wl,-Bstatic @TE_LDFLAGS@

bin_PROGRAMS = ta@with_name@

ta@with_name@_SOURCES = $(TA_SOURCES)
nodist_ta@with_name@_SOURCES = $(TA_NODIST_SOURCES)

ta@with_name@_LDADD = $(TA_LDADD) @EXTRA_LDADD@

SYMTBL_LIBS = \
    $(DESTDIR)/$(prefix)/../@host@/lib/libtools.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/librcfpch.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/lib@TAD@.a \
    conf/libconf.a rpc/libtarpc.a

ta@with_name@_DEPENDENCIES = @TE_DEPENDENCIES@ $(SYMTBL_LIBS) symtbl.o \
    $(DESTDIR)/$(prefix)/../@host@/lib/librpcxdr.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/libconf_oid.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/libloggerta.a 

symtbl.c: $(TA_SYMTBL_OBJECTS) $(SYMTBL_LIBS) @TE_DEPENDENCIES@
	$(srcdir)/../../engine/builder/te_generate_symtbl "${NM}" $@ $^ 

