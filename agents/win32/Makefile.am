
MY_CPPFLAGS=-I$(DESTDIR)/$(prefix)/../@host@/include -include config.h

#
# -lrcfpch must be before @TE_LDADD@ since the last may contain
# -ltad_ch with names used in -lrcfpch.
#
MY_LDADD=symtbl.o $(LIBLTDL) -lrcfpch -l@TAD@ @TE_LDADD@ -lconf_oid \
         -lcomm_net_agent -lloggerta -lpthread 
MY_LDAFLAGS=-rdynamic -export-dynamic 

MY_SOURCES=win32.c win32conf.c 
MY_SYMTBL_OBJECTS=win32.o 

noinst_HEADERS=win32_rpc.h 

CLEANFILES=symtbl.c

if CFG_RCF_RPC

MY_CPPFLAGS+=-DRCF_RPC -I.
MY_SOURCES+=win32_rpc.c tarpc_server.c 
MY_NODIST_SOURCES=tarpc_xdr.c tarpc_svc.c rpc_unsupported.c
MY_LDADD+=-lrpc -lws2_32 -lmswsock -lwsock32 -liphlpapi -lcygwin
MY_SYMTBL_OBJECTS+=win32_rpc.o tarpc_server.o

CLEANFILES+=tarpc.h tarpc_xdr.c tarpc_svc.c rpc_unsupported.c \
            rpc_supported.h

win32_rpc.o tarpc_server.o: tarpc.h

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

EXTRA_DIST=win32_rpc.h win32_rpc.c

endif


AM_CPPFLAGS=$(MY_CPPFLAGS)

tawin32_SOURCES=$(MY_SOURCES)
nodist_tawin32_SOURCES=$(MY_NODIST_SOURCES)

tawin32_LDADD=$(MY_LDADD)

tawin32_DEPENDENCIES=@TE_DEPENDENCIES@ symtbl.o \
    $(DESTDIR)/$(prefix)/../@host@/lib/librcfpch.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/lib@TAD@.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/libconf_oid.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/libcomm_net_agent.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/libloggerta.a

#
# Static linkage may be required, since agent may be copied to another
# host which does not have required dynamically linked libraries.
# Use -static.
#
AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/../@host@/lib


bin_PROGRAMS=tawin32

symtbl.c: $(MY_SYMTBL_OBJECTS) @TE_DEPENDENCIES@
	$(srcdir)/../../engine/builder/te_generate_symtbl "${NM}" $@ $^ 

install-exec-hook:
	cd $(DESTDIR)/$(prefix)/bin; ln -fs tawin32.exe tawin32

uninstall-hook:
	cd $(DESTDIR)/$(prefix)/bin; rm -f tawin32
