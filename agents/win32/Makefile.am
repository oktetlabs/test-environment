
ACLOCAL_AMFLAGS = -I @TE_M4@

MY_CPPFLAGS=@TE_CPPFLAGS@ -I$(DESTDIR)/$(prefix)/../@host@/include \
            -include te_config.h -include config.h -fno-strict-aliasing

#
# -lrcfpch must be before @TE_LDADD@ since the last may contain
# -ltad_ch with names used in -lrcfpch.
#
MY_LDADD=symtbl.o -lrcfpch -lrpcxdr \
         -l@TAD@ @TE_LDADD@ -lconf_oid -lloggerta -lpthread \
         -lws2_32 -lmswsock -lwsock32 -liphlpapi -lrpc -ltools -lcygwin \
         -lexpat 
         
MY_LDAFLAGS=-rdynamic -export-dynamic 

MY_SOURCES=win32.c win32conf.c inet_pton.c
MY_SYMTBL_OBJECTS=win32.o 

SYMTBL_LIBS= \
    $(DESTDIR)/$(prefix)/../@host@/lib/libtools.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/librcfpch.a 

CLEANFILES=symtbl.c

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
	$(srcdir)/../../engine/builder/te_rpc_supported "${NM}" tarpc_server.o

else

EXTRA_DIST=tarpc_server.c
        
endif


AM_CPPFLAGS=$(MY_CPPFLAGS)

tawin32tmpl_SOURCES=$(MY_SOURCES)
nodist_tawin32tmpl_SOURCES=$(MY_NODIST_SOURCES)

tawin32tmpl_LDADD=$(MY_LDADD) 

tawin32tmpl_DEPENDENCIES=@TE_DEPENDENCIES@ symtbl.o \
    $(DESTDIR)/$(prefix)/../@host@/lib/librcfpch.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/librpcxdr.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/lib@TAD@.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/libconf_oid.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/libcomm_net_agent.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/libloggerta.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/libtools.a

#
# Static linkage may be required, since agent may be copied to another
# host which does not have required dynamically linked libraries.
# Use -static.
#
AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/../@host@/lib


bin_PROGRAMS=tawin32tmpl

symtbl.c: $(MY_SYMTBL_OBJECTS) $(SYMTBL_LIBS) @TE_DEPENDENCIES@
	$(srcdir)/../../engine/builder/te_generate_symtbl "${NM}" $@ $^ 

install-exec-hook:
	mv $(DESTDIR)/$(bindir)/tawin32tmpl.exe \
		$(DESTDIR)/$(bindir)/ta@with_name@.exe
	ln -fs ta@with_name@.exe $(DESTDIR)/$(bindir)/ta@with_name@

uninstall-hook:
	rm -f $(DESTDIR)/$(bindir)/ta@with_name@ \
		$(DESTDIR)/$(bindir)/ta@with_name@.exe
