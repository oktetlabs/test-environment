
MY_CPPFLAGS=-I$(DESTDIR)/$(prefix)/../@host@/include \
            -I$(DESTDIR)/$(prefix)/../include

#
# -lrcfpch must be before @TE_LDADD@ since the last may contain
# -ltad_ch with names used in -lrcfpch.
#
MY_LDADD=symtbl.o $(LIBLTDL) -lrcfpch @TE_LDADD@ -lconf_oid \
         -lcomm_net_agent -lloggerta -lpthread 
MY_LDAFLAGS=-rdynamic -export-dynamic 

MY_SOURCES=win32.c win32conf.c ftp_routines.c
MY_SYMTBL_OBJECTS=win32.o ftp_routines.o

EXTRA_DIST=tarpc.x.m4

noinst_HEADERS=win32_rpc.h win32_rpc_log.h

if CFG_RCF_RPC

MY_CPPFLAGS+=-DRCF_RPC -I.
MY_SOURCES+=win32_rpc.c  tarpc_server.c 
MY_NODIST_SOURCES=tarpc_xdr.c tarpc_svc.c
MY_LDADD+=-lrpc -lws2_32 -lcygwin
MY_SYMTBL_OBJECTS+=win32_rpc.o tarpc_server.o

CLEANFILES=tarpc.h tarpc_xdr.c tarpc_svc.c

win32_rpc.o tarpc_server.o: tarpc.h

tarpc.h tarpc_xdr.c tarpc_svc.c: tarpc.x.m4
	rm -f tarpc.h tarpc_xdr.c tarpc_svc.c
	m4 $(srcdir)/tarpc.x.m4 > tarpc.x
	rpcgen -h -M tarpc.x -o tarpc.h
	rpcgen -c -M tarpc.x -o tarpc_xdr.c
	awk '\
            /buf;/ { print $0 ; printf("\t(void)buf;") ; next ; } \
            { print $0 ; next ;}' tarpc_xdr.c > tarpc_xdr.c.new
	mv tarpc_xdr.c.new tarpc_xdr.c
	rpcgen -m -M tarpc.x -o tarpc_svc.c 
	rm -f tarpc.x

else

EXTRA_DIST+=win32_rpc.h win32_rpc.c

endif


AM_CPPFLAGS=$(MY_CPPFLAGS)

tawin32_SOURCES=$(MY_SOURCES)
nodist_tawin32_SOURCES=$(MY_NODIST_SOURCES)

tawin32_LDADD=$(MY_LDADD)

tawin32_DEPENDENCIES=@TE_DEPENDENCIES@ symtbl.o \
    $(DESTDIR)/$(prefix)/../@host@/lib/librcfpch.a \
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

symtbl.o: $(MY_SYMTBL_OBJECTS) @TE_DEPENDENCIES@
	te_generate_symtbl_cygwin ${NM} symtbl.c $^ 
	${CC} -c -o $@ symtbl.c
