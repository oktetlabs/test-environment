
SUBDIRS=libltdl

MY_CPPFLAGS=-I$(DESTDIR)/$(prefix)/../@host@/include \
            -I$(DESTDIR)/$(prefix)/../include

MY_LDADD=symtbl.o $(LIBLTDL) @TE_LDADD@ -lpthread -lltdl
MY_LDAFLAGS=-rdynamic -export-dynamic 

MY_SOURCES=win32.c win32conf.c ftp_routines.c
MY_SYMTBL_OBJECTS=win32.o ftp_routines.o


#
# Temporary outside of CFG_RCF_RPC
#
etcdir=$(DESTDIR)/$(prefix)/etc/win32
etc_DATA=tarpc.x

EXTRA_DIST=tarpc.x.m4

tarpc.x: $(srcdir)/tarpc.x.m4
	m4 $(srcdir)/tarpc.x.m4 > tarpc.x

install-data-hook:
	mkdir -p $(etcdir) ; \
	touch -c -r $(srcdir)/tarpc.x.m4 $(etcdir)/tarpc.x ;

CLEANFILES=tarpc.x

noinst_HEADERS=win32_rpc.h win32_rpc_log.h

if CFG_RCF_RPC

noinst_HEADERS+=tarpc.h
MY_CPPFLAGS+=-DRCF_RPC -I.
MY_SOURCES+=win32_rpc.c tarpc_xdr.c tarpc_svc.c tarpc_server.c 
MY_LDADD+=-lrpc -lws2_32 -lcygwin
MY_SYMTBL_OBJECTS+=win32_rpc.o tarpc_server.o

CLEANFILES+=tarpc.h

win32_rpc.o tarpc_server.o: tarpc.h

tarpc.h: tarpc.x
	rm -f tarpc.h
	rpcgen -h -M tarpc.x -o tarpc.h

tarpc_xdr.c: tarpc.h
	rm -f tarpc_xdr.c
	rpcgen -c -M tarpc.x -o tarpc_xdr.c
	awk '\
            /buf;/ { print $0 ; printf("\t(void)buf;") ; next ; } \
            { print $0 ; next ;}' tarpc_xdr.c > tarpc_xdr.c.new
	mv tarpc_xdr.c.new tarpc_xdr.c

tarpc_svc.c: tarpc.h
	rm -f tarpc_svc.c
	rpcgen -m -M tarpc.x -o tarpc_svc.c 

else

EXTRA_DIST+=win32_rpc.h win32_rpc.c

endif


AM_CPPFLAGS=$(MY_CPPFLAGS)

tawin32_SOURCES=$(MY_SOURCES)

tawin32_LDADD=$(MY_LDADD)

tawin32_DEPENDENCIES=@TE_DEPENDENCIES@ symtbl.o


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
