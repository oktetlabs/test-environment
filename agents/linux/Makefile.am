
MY_CPPFLAGS=-I$(DESTDIR)/$(prefix)/include \
            -I$(DESTDIR)/$(prefix)/../@host@/include \
            -I$(DESTDIR)/$(prefix)/../include

#
# -lrcfpch must be before @TE_LDADD@ since the last may contain
# -ltad_ch with names used in -lrcfpch.
#
MY_LDADD=symtbl.o -lrcfpch @TE_LDADD@ -lconf_oid -lcomm_net_agent -lloggerta
MY_LDAFLAGS=-rdynamic -export-dynamic 

MY_SOURCES=linux.c linuxconf.c ftp_routines.c
MY_SYMTBL_OBJECTS=linux.o ftp_routines.o

EXTRA_DIST=tarpc.x.m4

noinst_HEADERS=linux_rpc.h linux_rpc_log.h

CLEANFILES=symtbl.c

if CFG_RCF_RPC

SUBDIRS = libltdl
MY_CPPFLAGS+=-DRCF_RPC -I.
MY_LDADD+=$(LIBLTDL) -lltdl
MY_SOURCES+=linux_rpc.c tarpc_server.c 
MY_NODIST_SOURCES=tarpc_xdr.c tarpc_svc.c
MY_SYMTBL_OBJECTS+=linux_rpc.o tarpc_server.o

CLEANFILES+=tarpc.h tarpc_xdr.c tarpc_svc.c 

linux_rpc.o tarpc_server.o: tarpc.h

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
	echo "#include <stdio.h>" > tarpc_svc.c.new
	cat tarpc_svc.c >> tarpc_svc.c.new
	mv tarpc_svc.c.new tarpc_svc.c
	rm -f tarpc.x

else

SUBDIRS=
DIST_SUBDIRS=

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

bin_PROGRAMS=talinuxtmpl

talinuxtmpl_SOURCES=$(MY_SOURCES)
nodist_talinuxtmpl_SOURCES=$(MY_NODIST_SOURCES)

talinuxtmpl_LDADD=$(MY_LDADD)

talinuxtmpl_DEPENDENCIES=@TE_DEPENDENCIES@ symtbl.o \
    $(DESTDIR)/$(prefix)/../@host@/lib/librcfpch.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/libconf_oid.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/libcomm_net_agent.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/libloggerta.a

symtbl.o: $(MY_SYMTBL_OBJECTS) @TE_DEPENDENCIES@
	if test -e $(DESTDIR)/$(prefix)/../@host@/bin/te_generate_symtbl ; then \
            $(DESTDIR)/$(prefix)/../@host@/bin/te_generate_symtbl "${NM}" symtbl.c $^ ; \
        else \
            te_generate_symtbl "${NM}" symtbl.c $^ ; \
        fi
	${CC} -c -o $@ symtbl.c

install-exec-hook:
	mv $(DESTDIR)/$(bindir)/talinuxtmpl $(DESTDIR)/$(bindir)/ta@with_name@

uninstall-hook:
	mv $(DESTDIR)/$(bindir)/ta@with_name@ $(DESTDIR)/$(bindir)/talinuxtmpl
