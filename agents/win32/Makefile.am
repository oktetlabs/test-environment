
ACLOCAL_AMFLAGS = -I @TE_M4@

MY_CPPFLAGS=@TE_CPPFLAGS@ -I$(DESTDIR)/$(prefix)/../@host@/include \
            -include te_config.h -include config.h -fno-strict-aliasing \
            -D'INCLUDE(x)='
            
MY_CFLAGS=--disable-stdcall-fixup            

#
# -lrcfpch must be before @TE_LDADD@ since the last may contain
# -ltad_ch with names used in -lrcfpch.
#
if CFG_RCF_RPC
MY_LDADD=symtbl.o -lrcfpch -lrpcxdrta \
         -l@TAD@ @TE_LDADD@ -lconf_oid -lloggerta -lpthread \
         -lws2_32 -lmswsock -lwsock32 -liphlpapi -lrpc -ltools -lcygwin \
         -lexpat 
else
MY_LDADD=symtbl.o -lrcfpch -lrpcxdr \
         -l@TAD@ @TE_LDADD@ -lconf_oid -lloggerta -lpthread \
         -lws2_32 -lmswsock -lwsock32 -liphlpapi -lrpc -ltools -lcygwin \
         -lexpat 
endif
         
noinst_HEADERS=tarpc_server.h 

MY_LDAFLAGS=-rdynamic -export-dynamic 
MY_SOURCES=win32.c win32conf.c inet_pton.c common.c
MY_SYMTBL_OBJECTS=win32.o 

SYMTBL_LIBS= \
    $(DESTDIR)/$(prefix)/../@host@/lib/libtools.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/librcfpch.a 

CLEANFILES=symtbl.c rpcserver_symtbl.c


if CFG_RCF_RPC

MY_CPPFLAGS+=-DRCF_RPC -I.
MY_SOURCES+=tarpc_server.c 
MY_NODIST_SOURCES=rpc_unsupported.c rpc_unsupported.h
MY_SYMTBL_OBJECTS+=tarpc_server.o

CLEANFILES+=rpc_unsupported.c rpc_supported.h

rpc_unsupported.c: $(DESTDIR)/$(prefix)/../@host@/include/tarpc.h \
                   rpc_supported.h \
                   $(srcdir)/../../include/rpc_unsupported.m4
	$(srcdir)/../../engine/builder/te_rpc_unsupported $< 
        
rpc_supported.h: tarpc_server.o @TE_DEPENDENCIES@
	$(srcdir)/../../engine/builder/te_rpc_supported "${NM}" $^

EXTRA_DIST=

else

EXTRA_DIST=tarpc_server.c
        
endif


AM_CPPFLAGS=$(MY_CPPFLAGS)

tawin32tmpl_SOURCES=$(MY_SOURCES)
nodist_tawin32tmpl_SOURCES=$(MY_NODIST_SOURCES)

tawin32tmpl_LDADD=$(MY_LDADD) 

tawin32tmpl_DEPENDENCIES=@TE_DEPENDENCIES@ symtbl.o \
    $(DESTDIR)/$(prefix)/../@host@/lib/librcfpch.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/librpcxdrta.a \
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

symtbl.c: $(MY_SYMTBL_OBJECTS) $(SYMTBL_LIBS) @TE_DEPENDENCIES@
	$(srcdir)/../../engine/builder/te_generate_symtbl "${NM}" $@ $^ 

if CFG_STANDALONE_RPCSERVER

bin_PROGRAMS=tawin32tmpl tawin32tmpl_rpcserver

tawin32tmpl_rpcserver_DEPENDENCIES=\
    rpcserver_symtbl.o \
    $(DESTDIR)/$(prefix)/../@host@/lib/librcfpch.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/libloggerta.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/libtools.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/libtalib.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/librpcxdrta.a 

tawin32tmpl_rpcserver_SOURCES=tarpc_server.c rpcserver.c common.c 

nodist_tawin32tmpl_rpcserver_SOURCES=$(MY_NODIST_SOURCES)

tawin32tmpl_rpcserver_LDADD=rpcserver_symtbl.o -ltalib -lrpcxdrta \
                            -lrcfpch -lloggerta -ltools -lrpc \
                            -lws2_32 -lmswsock -lwsock32 -liphlpapi 

rpcserver_symtbl.c: tarpc_server.o \
                    $(DESTDIR)/$(prefix)/../@host@/lib/libtalib.a
	$(srcdir)/../../engine/builder/te_generate_symtbl "${NM}" $@ $^ 

install-exec-hook: build_native
	mv $(DESTDIR)/$(bindir)/tawin32tmpl.exe \
		$(DESTDIR)/$(bindir)/ta@with_name@.exe
	ln -fs ta@with_name@.exe $(DESTDIR)/$(bindir)/ta@with_name@
	mv $(DESTDIR)/$(bindir)/tawin32tmpl_rpcserver.exe \
		$(DESTDIR)/$(bindir)/ta@with_name@_rpcserver.exe

uninstall-hook:
	rm -f $(DESTDIR)/$(bindir)/ta@with_name@ \
	      $(DESTDIR)/$(bindir)/ta@with_name@.exe \
	      $(DESTDIR)/$(bindir)/ta@with_name@_rpcserver*

RPCSERVER_LIBS="librcfpch.lib libloggerta.lib libtools.lib librpc.lib \
                ws2_32.lib mswsock.lib wsock32.lib iphlpapi.lib user32.lib"

build_native: $(tawin32tmpl_rpcserver_SOURCES) \
              rpcserver_symtbl.c rpc_unsupported.c \
              ${srcdir}/../../lib/rpcxdr/rpc_xdr.c \
              ../../lib/@_host_@_rpcxdr/tarpc_xdr.c \
              @PRIV_LIB_DIR@/talib/rpc.c
	TE_INSTALL=$(DESTDIR)/$(prefix)/.. \
        $(srcdir)/../../engine/builder/te_build_win_app $(srcdir) \
            i686-pc-windows ta@with_name@_rpcserver.exe \
            \"$(RPCSERVER_LIBS)\" $^ 
	if test -e ta@with_name@_rpcserver.exe ; then \
            cp ta@with_name@_rpcserver.exe $(DESTDIR)/$(bindir) ; \
        fi
	TE_INSTALL=$(DESTDIR)/$(prefix)/.. \
        $(srcdir)/../../engine/builder/te_build_win_app $(srcdir) \
            x86_64-pc-windows ta@with_name@_rpcserver64.exe \
            \"$(RPCSERVER_LIBS)\" $^
	if test -e ta@with_name@_rpcserver64.exe ; then \
            cp ta@with_name@_rpcserver64.exe $(DESTDIR)/$(bindir) ; \
        fi

else

# Built-in RPC server

bin_PROGRAMS=tawin32tmpl 

EXTRA_DIST+=rpcserver.c

install-exec-hook:
	mv $(DESTDIR)/$(bindir)/tawin32tmpl.exe \
		$(DESTDIR)/$(bindir)/ta@with_name@.exe
	ln -fs ta@with_name@.exe $(DESTDIR)/$(bindir)/ta@with_name@

uninstall-hook:
	rm -f $(DESTDIR)/$(bindir)/ta@with_name@ \
		$(DESTDIR)/$(bindir)/ta@with_name@.exe 

endif
