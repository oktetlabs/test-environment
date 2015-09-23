
SUBDIRS=rpc conf 

ACLOCAL_AMFLAGS = -I $(TE_M4)

AM_CPPFLAGS = -I$(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/include \
              $(TE_CPPFLAGS) \
              $(EXTRA_CPPFLAGS)

AM_CFLAGS = -fno-strict-aliasing $(TE_CFLAGS)

AM_LDFLAGS = -L$(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib \
             -Wl,-Bstatic $(TE_LDFLAGS)

LIBS = @LIBS@

EXTRA_DIST=symtbl_funcs

taexecdir=$(prefix)/@with_name@

noinst_HEADERS = unix_internal.h dahdi_user.h

taexec_PROGRAMS = ta

ta_extra_sources =
if WITH_TELEPHONY
ta_extra_sources += telephony.c
endif

if WITH_POWER_SW
ta_extra_sources += power_sw.c
endif

ta_SOURCES = main.c ftp_routines.c log_remote.c aux_threads.c \
             $(ta_extra_sources)

ta_LDADD = symtbl.o conf/libconf.a rpc/libtarpc.a -lrcfpch -lm \
           -l$(TAD) $(TE_LDADD) \
           -lrpcxdr -lrpc_types -lconf_oid -lloggerta -lrcfpch -ltools \
           $(EXTRA_LDADD)

CLEANFILES = symtbl.c

SYMTBL_OBJECTS = main.o ftp_routines.o log_remote.o
SYMTBL_LIBS = $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libtools.a \
              $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/librcfpch.a \
              $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/lib$(TAD).a \
              conf/libconf.a rpc/libtarpc.a

if WITH_TELEPHONY
SYMTBL_OBJECTS += telephony.o
endif

if WITH_POWER_SW
SYMTBL_OBJECTS += power_sw.o
endif

if ISCSI_TARGET_USE
SYMTBL_LIBS+=\
$(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libiscsi_unh_target.a
endif      

if CFG_SNIFFERS_DUMMY
ta_LDADD+= -lrcf_sniffers_dummy
endif

SYMTBL_LIBS+=@CURL_LIB@

ta_DEPENDENCIES = @TE_DEPENDENCIES@ $(SYMTBL_LIBS) symtbl.o \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/librpcxdr.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/librpc_types.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libconf_oid.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libloggerta.a 

if CFG_TR069
ta_DEPENDENCIES += $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libacse_cwmp.a
SYMTBL_OBJECTS += $(TE_BUILD)/lib/acse/acse_rpc.o 
endif

symtbl.c: $(SYMTBL_OBJECTS) $(SYMTBL_LIBS) @TE_DEPENDENCIES@ \
          $(srcdir)/symtbl_funcs
	NM_FORMAT=@NM_FORMAT@ $(srcdir)/../../engine/builder/te_generate_symtbl \
		"${NM}" \
		empty $(srcdir)/symtbl_funcs $@ \
		$(SYMTBL_OBJECTS) $(SYMTBL_LIBS) @TE_DEPENDENCIES@

ta_SOURCES += rcf_consistency_checks.c

.PHONY: rcf_consistency_checks.c
rcf_consistency_checks.c:
	TE_BASE="@TE_BASE@" \
	TE_BUILD="@TE_BUILD@" \
	"@TE_BASE@/engine/builder/te_rcf_consistency_checks" \
		--generate-c >$@
