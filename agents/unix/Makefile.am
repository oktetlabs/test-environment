
ACLOCAL_AMFLAGS = -I $(TE_M4)

AM_CPPFLAGS = -I$(DESTDIR)/$(prefix)/../@host@/include $(TE_CPPFLAGS) \
              -include te_config.h -include config.h
AM_CFLAGS = -fno-strict-aliasing $(TE_CFLAGS)

AM_YFLAGS = -d

#
# -lrcfpch must be before -l@TAD@ since rcfpch contains references to
# functions provided by -l@TAD@. @TE_LDADD@ must be after -l@TAD@,
# since @TE_LDADD@ may contain ASN and NDN libraries.
#
TA_LDADD = symtbl.o -lrcfpch -l@TAD@ @TE_LDADD@ -lrpcxdr -lrpc_types \
           -lconf_oid -lloggerta -lrcfpch -ltools

noinst_HEADERS = \
	unix_internal.h \
	conf_route.h \
	conf_daemons.h \
	tarpc_server.h

TA_SOURCES = \
	main.c \
	conf.c \
	conf_stats.c \
	conf_route.c \
	conf_route_netlink.c \
	conf_route_socket.c \
	conf_getmsg.c \
	ftp_routines.c \
	log_serial.c

TA_SYMTBL_OBJECTS = main.o ftp_routines.o log_serial.o 

CLEANFILES = symtbl.c
BUILT_SOURCES =

if CFG_NETLINK_USE
TA_LDADD += -lnetlink
endif


if ISCSI_TARGET_USE

AM_CPPFLAGS += -I$(DESTDIR)/$(prefix)/../@host@/include/iscsi_unh_target 
TA_SOURCES += conf_iscsi_target.c 
TA_LDADD += -liscsi_unh_target -liscsi_initiator_conf
TA_SYMTBL_OBJECTS += \
	$(DESTDIR)/$(prefix)/../@host@/lib/libiscsi_unh_target.a

endif


if CFG_RCF_RPC

AM_CPPFLAGS += -DRCF_RPC
TA_SOURCES += tarpc_server.c 
TA_NODIST_SOURCES = rpc_unsupported.c rpc_unsupported.h
TA_SYMTBL_OBJECTS += tarpc_server.o

CLEANFILES += rpc_unsupported.c rpc_supported.h

rpc_unsupported.c: $(DESTDIR)/$(prefix)/../@host@/include/tarpc.h \
                   rpc_supported.h \
                   $(srcdir)/../../include/rpc_unsupported.m4
	$(srcdir)/../../engine/builder/te_rpc_unsupported $< 
        
rpc_supported.h: tarpc_server.o @TE_DEPENDENCIES@
	$(srcdir)/../../engine/builder/te_rpc_supported "${NM}" $^

else

EXTRA_DIST = tarpc_server.c
        
endif


if CFG_UNIX_DAEMONS

AM_CPPFLAGS += @DAEMONS_CFLAGS@

noinst_HEADERS += dhcp_server.h

TA_SOURCES += conf_daemons.c dhcp_server.c dns_server.c 
TA_SYMTBL_OBJECTS += conf_daemons.o

TA_LDADD += @DAEMONS_LDADD@

if CFG_UNIX_DAEMONS_NAMED

TA_SOURCES += named.conf.y named.lex.l

BUILT_SOURCES += named.conf.h
CLEANFILES += named.conf.h
TA_LDADD += -lfl

endif

if CFG_UNIX_DAEMONS_ISC_DHCP_SERVER
#TA_SOURCES += isc-dhcp-server.conf.y isc-dhcp-server.lex.l
#BUILT_SOURCES += isc-dhcp-server.conf.h
#TA_LDADD += -lfl
endif

endif

if CFG_UNIX_DAEMONS_RADIUS
TA_SOURCES += radius.c
endif

if CFG_UNIX_8021X
TA_SOURCES += conf_8021x.c
endif

if CFG_UNIX_WIFI
TA_SOURCES += conf_wifi.c
endif

if CFG_UNIX_DAEMONS_VTUND
TA_SOURCES += conf_vtund.c
endif

LIBS = -Wl,-Bdynamic @TAD_LIBS@ @LIBS@

AM_CPPFLAGS += @EXTRA_CPPFLAGS@

AM_LDFLAGS = -L$(DESTDIR)/$(prefix)/../@host@/lib -Wl,-Bstatic @TE_LDFLAGS@

bin_PROGRAMS = ta@with_name@

ta@with_name@_SOURCES = $(TA_SOURCES)
nodist_ta@with_name@_SOURCES = $(TA_NODIST_SOURCES)

ta@with_name@_LDADD = $(TA_LDADD)

SYMTBL_LIBS = \
	$(DESTDIR)/$(prefix)/../@host@/lib/libtools.a \
	$(DESTDIR)/$(prefix)/../@host@/lib/librcfpch.a \
	$(DESTDIR)/$(prefix)/../@host@/lib/lib@TAD@.a

ta@with_name@_DEPENDENCIES = @TE_DEPENDENCIES@ $(SYMTBL_LIBS) symtbl.o \
	$(DESTDIR)/$(prefix)/../@host@/lib/librpcxdr.a \
	$(DESTDIR)/$(prefix)/../@host@/lib/libconf_oid.a \
	$(DESTDIR)/$(prefix)/../@host@/lib/libloggerta.a

symtbl.c: $(TA_SYMTBL_OBJECTS) $(SYMTBL_LIBS) @TE_DEPENDENCIES@
	$(srcdir)/../../engine/builder/te_generate_symtbl "${NM}" $@ $^ 
