
SUBDIRS=libltdl

MY_CPPFLAGS=-I$(DESTDIR)/$(prefix)/include \
            -I$(DESTDIR)/$(prefix)/../@host@/include \
            -I$(DESTDIR)/$(prefix)/../include

MY_LDADD=symtbl.o $(LIBLTDL) @TE_LDADD@ -lltdl @PTHREAD_LDADD@
MY_LDAFLAGS=-rdynamic -export-dynamic 

MY_SOURCES=linux.c linuxconf.c ftp_routines.c
MY_SYMTBL_OBJECTS=linux.o ftp_routines.o


#
# Temporary outside of CFG_RCF_RPC
#
etcdir=$(DESTDIR)/$(prefix)/etc/linux
etc_DATA=tarpc.x

EXTRA_DIST=tarpc.x.m4

tarpc.x: $(srcdir)/tarpc.x.m4
	m4 $(srcdir)/tarpc.x.m4 > tarpc.x

install-data-hook:
	mkdir -p $(etcdir) ; \
	touch -c -r $(srcdir)/tarpc.x.m4 $(etcdir)/tarpc.x ;

CLEANFILES=tarpc.x

noinst_HEADERS=linux_rpc.h linux_rpc_log.h

if CFG_RCF_RPC

noinst_HEADERS+=tarpc.h
MY_CPPFLAGS+=-DRCF_RPC -I.
MY_SOURCES+=linux_rpc.c tarpc_xdr.c tarpc_svc.c tarpc_server.c 
MY_LDADD+=@AIO_LDADD@
MY_SYMTBL_OBJECTS+=linux_rpc.o tarpc_server.o

CLEANFILES+=tarpc.h

linux_rpc.o tarpc_server.o: tarpc.h

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
	echo "#include <stdio.h>" > tarpc_svc.c.new
	cat tarpc_svc.c >> tarpc_svc.c.new
	mv tarpc_svc.c.new tarpc_svc.c

else

EXTRA_DIST+=linux_rpc.h linux_rpc.c

endif


if CFG_LINUX_DAEMONS

MY_CPPFLAGS+=@DAEMONS_CFLAGS@

MY_SOURCES+=linuxconf_daemons.c 

MY_LDADD+=@DAEMONS_LDADD@ -ldl

endif


AM_CPPFLAGS=$(MY_CPPFLAGS)


#
# Static linkage may be required, since agent may be copied to another
# host which does not have required dynamically linked libraries.
# Use -static.
#
AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/../@host@/lib

if TALINUX

talinux_SOURCES=$(MY_SOURCES)

talinux_LDADD=$(MY_LDADD)

talinux_DEPENDENCIES=@TE_DEPENDENCIES@ symtbl.o

bin_PROGRAMS=talinux

else

tafreebsd_SOURCES=$(MY_SOURCES)

tafreebsd_LDADD=$(MY_LDADD)

tafreebsd_DEPENDENCIES=@TE_DEPENDENCIES@ symtbl.o

bin_PROGRAMS=tafreebsd

endif

symtbl.o: $(MY_SYMTBL_OBJECTS) @TE_DEPENDENCIES@
	te_generate_symtbl ${NM} symtbl.c $^ 
	${CC} -c -o $@ symtbl.c
