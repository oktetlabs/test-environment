
ACLOCAL_AMFLAGS = -I @TE_M4@

MY_CPPFLAGS=@TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) \
            -I$(DESTDIR)/$(prefix)/../@host@/include \
            -include te_config.h \
            -include config.h -fno-strict-aliasing 

AM_YFLAGS=-d

#
# -lrcfpch must be before -l@TAD@ since rcfpch contains references to
# functions provided by -l@TAD@. @TE_LDADD@ must be after -l@TAD@,
# since @TE_LDADD@ may contain ASN and NDN libraries.
#
MY_LDADD=symtbl.o @TE_LDADD@ -lrcfpch -lrpcxdr -l@TAD@ \
         -lconf_oid -lloggerta -lrcfpch -ltools

#if HAS_NET_SNMP
#MY_LDADD+= @NET_SNMP_LIBS@
#endif

MY_SOURCES=main.c conf.c ftp_routines.c log_serial.c
MY_SYMTBL_OBJECTS=main.o ftp_routines.o log_serial.o 

noinst_HEADERS=unix_internal.h conf_daemons.h tarpc_server.h

CLEANFILES=symtbl.c
BUILT_SOURCES=

if CFG_NETLINK_USE
MY_LDADD+=-lnetlink
endif

if ISCSI_TARGET_USE
MY_CPPFLAGS+=-I$(DESTDIR)/$(prefix)/../@host@/include/iscsi_unh_target
MY_SOURCES+=conf_iscsi_target.c conf_iscsi_initiator.c
MY_LDADD+=-liscsi_unh_target
MY_SYMTBL_OBJECTS+=$(DESTDIR)/$(prefix)/../@host@/lib/libiscsi_unh_target.a
endif


if CFG_RCF_RPC

MY_CPPFLAGS+=-DRCF_RPC -I.
MY_SOURCES+=tarpc_server.c 
MY_NODIST_SOURCES=rpc_unsupported.c rpc_unsupported.h
MY_SYMTBL_OBJECTS+=tarpc_server.o

CLEANFILES+=rpc_unsupported.c rpc_supported.h

rpc_unsupported.c: $(srcdir)/../../include/tarpc.x.m4 rpc_supported.h \
                   $(srcdir)/../../include/rpc_unsupported.m4
	cat $< | grep 'RPC_DEF(' > tmp
	cat $(srcdir)/../../include/rpc_unsupported.m4 tmp | m4 > $@
	rm -f tmp
        
rpc_supported.h: tarpc_server.o
	$(srcdir)/../../engine/builder/te_rpc_supported "${NM}" \
            tarpc_server.o @TE_DEPENDENCIES@

else

EXTRA_DIST=tarpc_server.c
        
endif


if CFG_UNIX_DAEMONS

MY_CPPFLAGS+=@DAEMONS_CFLAGS@

noinst_HEADERS+=dhcp_server.h

MY_SOURCES+=conf_daemons.c dhcp_server.c dns_server.c 
MY_SYMTBL_OBJECTS+=conf_daemons.o

MY_LDADD+=@DAEMONS_LDADD@

if CFG_UNIX_DAEMONS_NAMED

MY_SOURCES+=named.conf.y named.lex.l

BUILT_SOURCES+=named.conf.h
CLEANFILES+=named.conf.h
MY_LDADD+=-lfl

endif

if CFG_UNIX_DAEMONS_ISC_DHCP_SERVER
#MY_SOURCES+=isc-dhcp-server.conf.y isc-dhcp-server.lex.l
#BUILT_SOURCES+=isc-dhcp-server.conf.h
#MY_LDADD+=-lfl
endif

endif

if CFG_UNIX_AUTH
MY_SOURCES+=radius.c
endif

if CFG_UNIX_WIFI
MY_SOURCES+=conf_wifi.c
endif

if CFG_UNIX_DAEMONS_VTUND
MY_SOURCES+=conf_vtund.c
endif

LIBS+=@TAD_LIBS@

AM_CPPFLAGS = $(MY_CPPFLAGS) @EXTRA_CPPFLAGS@
AM_CFLAGS = @TE_CFLAGS@ 

#
# Static linkage may be required, since agent may be copied to another
# host which does not have required dynamically linked libraries.
# Use -static.
#
AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/../@host@/lib

bin_PROGRAMS=taunixtmpl

taunixtmpl_SOURCES=$(MY_SOURCES)
nodist_taunixtmpl_SOURCES=$(MY_NODIST_SOURCES)

taunixtmpl_LDADD=$(MY_LDADD)

taunixtmpl_DEPENDENCIES=@TE_DEPENDENCIES@ symtbl.o \
    $(DESTDIR)/$(prefix)/../@host@/lib/libtools.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/librcfpch.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/librpcxdr.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/lib@TAD@.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/libconf_oid.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/libloggerta.a

SYMTBL_LIBS= \
    $(DESTDIR)/$(prefix)/../@host@/lib/libtools.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/librcfpch.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/lib@TAD@.a

symtbl.c: $(MY_SYMTBL_OBJECTS) $(SYMTBL_LIBS) @TE_DEPENDENCIES@
	$(srcdir)/../../engine/builder/te_generate_symtbl "${NM}" $@ $^ 

install-exec-hook:
	mv $(DESTDIR)/$(bindir)/taunixtmpl $(DESTDIR)/$(bindir)/ta@with_name@

uninstall-hook:
	rm -f $(DESTDIR)/$(bindir)/ta@with_name@
