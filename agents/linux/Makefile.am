
MY_CPPFLAGS=-I$(DESTDIR)/$(prefix)/include \
            -I$(DESTDIR)/$(prefix)/../@host@/include \
            -include te_config.h \
            -include config.h -fno-strict-aliasing

AM_YFLAGS=-d

#
# -lrcfpch must be before -l@TAD@ since rcfpch contains references to
# functions provided by -l@TAD@. @TE_LDADD@ must be after -l@TAD@,
# since @TE_LDADD@ may contain ASN and NDN libraries.
#
MY_LDADD=symtbl.o -lrcfpch -llogfork -l@TAD@ @TE_LDADD@ \
         -lconf_oid -lloggerta 

#if HAS_NET_SNMP
#MY_LDADD+= @NET_SNMP_LIBS@
#endif

MY_SOURCES=linux.c linuxconf.c ftp_routines.c log_serial.c
MY_SYMTBL_OBJECTS=linux.o ftp_routines.o log_serial.o 

noinst_HEADERS=linux_rpc.h linux_internal.h linuxconf_daemons.h

CLEANFILES=symtbl.c
BUILT_SOURCES=

if CFG_RCF_RPC

MY_CPPFLAGS+=-DRCF_RPC -I.
MY_SOURCES+=linux_rpc.c tarpc_server.c 
MY_NODIST_SOURCES=tarpc_xdr.c tarpc_svc.c rpc_unsupported.c
MY_SYMTBL_OBJECTS+=linux_rpc.o tarpc_server.o

CLEANFILES+=tarpc.h tarpc_xdr.c tarpc_svc.c rpc_unsupported.c \
            rpc_supported.h

linux_rpc.o tarpc_server.o: tarpc.h

tarpc.h tarpc_xdr.c tarpc_svc.c: $(srcdir)/../../include/tarpc.x.m4
	rm -f tarpc.h tarpc_xdr.c tarpc_svc.c
	m4 $< > tarpc.x
	rpcgen -h -M tarpc.x -o tarpc.h
	rpcgen -c -M tarpc.x -o tarpc_xdr.c
	awk '\
            /buf;/ { print $0 ; printf("\t(void)buf;") ; next ; } \
            { print $0 ; next ;}' tarpc_xdr.c > tarpc_xdr.c.new
	mv tarpc_xdr.c.new tarpc_xdr.c
	rpcgen -m -M tarpc.x -o tarpc_svc.c 
	echo "#include <stdio.h>" > tarpc_svc.c.new
	cat tarpc_svc.c >> tarpc_svc.c.new
	mv tarpc_svc.c.new tarpc_svc.c
	rm -f tarpc.x

rpc_unsupported.c: $(srcdir)/../../include/tarpc.x.m4 rpc_supported.h \
                   $(srcdir)/../../include/rpc_unsupported.m4
	cat $< | grep 'RPC_DEF(' > tmp
	cat $(srcdir)/../../include/rpc_unsupported.m4 tmp | m4 > $@
	rm -f tmp
        
rpc_supported.h: tarpc_server.o
	$(srcdir)/../../engine/builder/te_rpc_supported "${NM}" 
        
else

EXTRA_DIST=linux_rpc.h linux_rpc.c

endif


if CFG_LINUX_DAEMONS

MY_CPPFLAGS+=@DAEMONS_CFLAGS@

noinst_HEADERS+=dhcp_server.h

MY_SOURCES+=linuxconf_daemons.c dhcp_server.c dns_server.c 
MY_SYMTBL_OBJECTS+=linuxconf_daemons.o

MY_LDADD+=@DAEMONS_LDADD@

if CFG_LINUX_DAEMONS_NAMED

MY_SOURCES+=named.conf.y named.lex.l

BUILT_SOURCES+=named.conf.h
CLEANFILES+=named.conf.h
MY_LDADD+=-lfl

endif

if CFG_LINUX_DAEMONS_ISC_DHCP_SERVER
#MY_SOURCES+=isc-dhcp-server.conf.y isc-dhcp-server.lex.l
#BUILT_SOURCES+=isc-dhcp-server.conf.h
#MY_LDADD+=-lfl
endif

endif

LIBS+=@TAD_LIBS@

AM_CPPFLAGS=$(MY_CPPFLAGS)


#
# Static linkage may be required, since agent may be copied to another
# host which does not have required dynamically linked libraries.
# Use -static.
#
AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/../@host@/lib

bin_PROGRAMS=talinuxtmpl

talinuxtmpl_SOURCES=$(MY_SOURCES)
nodist_talinuxtmpl_SOURCES=$(MY_NODIST_SOURCES)

talinuxtmpl_LDADD=$(MY_LDADD)

talinuxtmpl_DEPENDENCIES=@TE_DEPENDENCIES@ symtbl.o \
    $(DESTDIR)/$(prefix)/../@host@/lib/librcfpch.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/liblogfork.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/lib@TAD@.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/libconf_oid.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/libloggerta.a

SYMTBL_LIBS= $(DESTDIR)/$(prefix)/../@host@/lib/lib@TAD@.a

symtbl.c: $(MY_SYMTBL_OBJECTS) $(SYMTBL_LIBS) @TE_DEPENDENCIES@
	$(srcdir)/../../engine/builder/te_generate_symtbl "${NM}" $@ $^ 

install-exec-hook:
	mv $(DESTDIR)/$(bindir)/talinuxtmpl $(DESTDIR)/$(bindir)/ta@with_name@

uninstall-hook:
	mv $(DESTDIR)/$(bindir)/ta@with_name@ $(DESTDIR)/$(bindir)/talinuxtmpl
