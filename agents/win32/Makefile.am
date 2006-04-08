
ACLOCAL_AMFLAGS = -I $(TE_M4)
AM_CPPFLAGS=$(TE_CPPFLAGS) -I$(DESTDIR)/$(prefix)/../@host@/include \
            -include te_config.h -include config.h -fno-strict-aliasing \
            -D_WIN32_WINNT=0x0501 -D'INCLUDE(x)='
AM_CFLAGS=$(TE_CFLAGS)
AM_LDFLAGS=$(TE_LDFLAGS) -L$(DESTDIR)/$(prefix)/../@host@/lib

noinst_HEADERS=tarpc_server.h 

ta@with_name@_SOURCES=win32.c win32conf.c inet_pton.c common.c

ta@with_name@_DEPENDENCIES=$(TE_DEPENDENCIES) symtbl.o \
    $(DESTDIR)/$(prefix)/../@host@/lib/librcfpch.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/librpcxdrta.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/libconf_oid.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/libcomm_net_agent.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/libloggerta.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/libtools.a

EXTRA_DIST=
CLEANFILES=symtbl.c

SYMTBL_OBJECTS=win32.o 
SYMTBL_LIBS= $(DESTDIR)/$(prefix)/../@host@/lib/libtools.a \
             $(DESTDIR)/$(prefix)/../@host@/lib/librcfpch.a 

if CFG_RCF_RPC

LDADD=symtbl.o \
-lrcfpch -lrpcxdrta -l$(TAD) $(TE_LDADD) -lconf_oid -lloggerta -lpthread \
-lws2_32 -lmswsock -lwsock32 -liphlpapi -lrpc -ltools -lcygwin -lexpat

AM_CPPFLAGS+=-DRCF_RPC -I.
ta@with_name@_SOURCES+=tarpc_server.c rpc_unsupported.c
BUILT_SOURCES=rpc_unsupported.c rpc_supported.h
SYMTBL_OBJECTS+=tarpc_server.o

CLEANFILES+=rpc_unsupported.c rpc_supported.h

rpc_unsupported.c: $(DESTDIR)/$(prefix)/../@host@/include/tarpc.h \
                   rpc_supported.h \
                   $(srcdir)/../../include/rpc_unsupported.m4
	$(srcdir)/../../engine/builder/te_rpc_unsupported $< 
        
rpc_supported.h: tarpc_server.o @TE_DEPENDENCIES@
	$(srcdir)/../../engine/builder/te_rpc_supported "${NM}" $^


if CFG_STANDALONE_RPCSERVER

bin_PROGRAMS=ta@with_name@ ta@with_name@_rpcserver \
             ta@with_name@_rpcserver32 ta@with_name@_rpcserver64

CLEANFILES+=rpcserver_symtbl.c

ta@with_name@_rpcserver_DEPENDENCIES=\
    rpcserver_symtbl.o \
    $(DESTDIR)/$(prefix)/../@host@/lib/librcfpch.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/libloggerta.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/libtools.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/libtalib.a \
    $(DESTDIR)/$(prefix)/../@host@/lib/librpcxdrta.a 

ta@with_name@_rpcserver_SOURCES=tarpc_server.c rpcserver.c \
                                common.c rpc_unsupported.c

ta@with_name@_rpcserver_LDADD=rpcserver_symtbl.o $(TE_LDADD) -lrpcxdrta \
                              -lrcfpch -lloggerta -ltools -lrpc \
                              -lws2_32 -lmswsock -lwsock32 -liphlpapi 

rpcserver_symtbl.c: tarpc_server.o \
                    $(DESTDIR)/$(prefix)/../@host@/lib/libtalib.a
	$(srcdir)/../../engine/builder/te_generate_symtbl "${NM}" $@ $^ 

# Native build

RPCSERVER_NAME=ta@with_name@_rpcserver$(EXEEXT)
RPCSERVER32_NAME=ta@with_name@_rpcserver32$(EXEEXT)
RPCSERVER64_NAME=ta@with_name@_rpcserver64$(EXEEXT)

RPCSERVER_LIBS="librcfpch.a libloggerta.a libtools.a librpc.a \
                ws2_32.lib mswsock.lib wsock32.lib iphlpapi.lib user32.lib"

RPCSERVER_SRC= \
    $(ta@with_name@_rpcserver_SOURCES) \
    rpcserver_symtbl.c rpc_unsupported.c \
    ${srcdir}/../../lib/rpcxdr/rpc_xdr.c \
    ../../lib/@_host_@_rpcxdr/tarpc_xdr.c \
    @PRIV_LIB_DIR@/talib/rpc.c

ta@with_name@_rpcserver32$(EXEEXT): $(RPCSERVER_SRC)        
	TE_INSTALL=$(DESTDIR)/$(prefix)/.. \
        $(srcdir)/../../engine/builder/te_build_win_app $(srcdir) \
        i686-pc-windows $@ \"$(RPCSERVER_LIBS)\" $^
        
ta@with_name@_rpcserver64$(EXEEXT): $(RPCSERVER_SRC)        
	TE_INSTALL=$(DESTDIR)/$(prefix)/.. \
        $(srcdir)/../../engine/builder/te_build_win_app $(srcdir) \
        x86_64-pc-windows $@ \"$(RPCSERVER_LIBS)\" $^

install-exec-hook:
	ln -fs ta@with_name@.exe $(DESTDIR)/$(bindir)/ta@with_name@
	if test -e $(RPCSERVER32_NAME) -a ! -s $(RPCSERVER32_NAME) \
                -a -n "$$TE_WIN32_BUILD_HOST" ; then \
            rm ta@with_name@_rpcserver* ; $(MAKE) install ; \
        fi
	if test ! -s $(DESTDIR)/$(bindir)/$(RPCSERVER32_NAME) ; then \
            rm -f $(DESTDIR)/$(bindir)/$(RPCSERVER32_NAME) ; \
        fi
	if test ! -s $(DESTDIR)/$(bindir)/$(RPCSERVER64_NAME) ; then \
            rm -f $(DESTDIR)/$(bindir)/$(RPCSERVER64_NAME) ; \
        fi
	if test -s $(DESTDIR)/$(bindir)/$(RPCSERVER32_NAME) ; then \
            mv $(DESTDIR)/$(bindir)/$(RPCSERVER32_NAME) \
               $(DESTDIR)/$(bindir)/$(RPCSERVER_NAME) ; \
        fi

clean-local:
	rm -f *.exe *.tgz

distclean-local:
	rm -f *.exe *.tgz

else

# Built-in RPC server

bin_PROGRAMS=ta@with_name@ 

EXTRA_DIST+=rpcserver.c

install-exec-hook:
	ln -fs ta@with_name@.exe $(DESTDIR)/$(bindir)/ta@with_name@

endif

else

# No RPC

bin_PROGRAMS=ta@with_name@ 

LDADD=symtbl.o \
-lrcfpch -lrpcxdr -l$(TAD) $(TE_LDADD) -lconf_oid -lloggerta -lpthread \
-lws2_32 -lmswsock -lwsock32 -liphlpapi -lrpc -ltools -lcygwin -lexpat 

EXTRA_DIST+=tarpc_server.c rpcserver.c

install-exec-hook:
	ln -fs ta@with_name@.exe $(DESTDIR)/$(bindir)/ta@with_name@
        
endif

# TA symbol table

symtbl.c: $(SYMTBL_OBJECTS) $(SYMTBL_LIBS) $(TE_DEPENDENCIES)
	$(srcdir)/../../engine/builder/te_generate_symtbl "${NM}" $@ $^ 

uninstall-hook:
	rm -f $(DESTDIR)/$(bindir)/ta@with_name@ 
