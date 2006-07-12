
ACLOCAL_AMFLAGS = -I $(TE_M4)
AM_CPPFLAGS=$(TE_CPPFLAGS) -I$(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/include \
            -include te_config.h -include config.h -fno-strict-aliasing \
            -D_WIN32_WINNT=0x0501 -D'INCLUDE(x)='
AM_CFLAGS=$(TE_CFLAGS)
AM_LDFLAGS=$(TE_LDFLAGS) -L$(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib

tadir=$(prefix)/@with_name@

noinst_HEADERS=tarpc_server.h 

ta_SOURCES=win32.c win32conf.c inet_pton.c inet_ntop.c common.c

ta_DEPENDENCIES=$(TE_DEPENDENCIES) symtbl.o \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/librcfpch.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/librpcxdrta.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/librpc_types.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libconf_oid.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libcomm_net_agent.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libloggerta.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libtools.a

CLEANFILES=symtbl.c

SYMTBL_OBJECTS=win32.o 
SYMTBL_LIBS= $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libtools.a \
             $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/librcfpch.a

if CFG_RCF_RPC

LDADD=symtbl.o \
-lrcfpch -lrpc_types -lrpcxdrta -l$(TAD) $(TE_LDADD) -lconf_oid -lloggerta \
-lpthread -lws2_32 -lmswsock -lwsock32 -liphlpapi -lrpc -ltools -lcygwin \
-lexpat

AM_CPPFLAGS+=-DRCF_RPC -I.
ta_SOURCES+=tarpc_server.c 
nodist_ta_SOURCES=rpc_unsupported.c rpc_supported.h

BUILT_SOURCES=rpc_unsupported.c rpc_supported.h

SYMTBL_OBJECTS+=tarpc_server.o common.o

CLEANFILES+=rpc_unsupported.c rpc_supported.h

rpc_unsupported.c: $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/include/tarpc.h \
                   rpc_supported.h \
                   $(srcdir)/../../include/rpc_unsupported.m4
	$(srcdir)/../../engine/builder/te_rpc_unsupported $< 
        
rpc_supported.h: tarpc_server.o @TE_DEPENDENCIES@
	$(srcdir)/../../engine/builder/te_rpc_supported "${NM}" $^


if CFG_STANDALONE_RPCSERVER

ta_PROGRAMS=ta rpcserver rpcserver32 rpcserver64
 
CLEANFILES+=rpcserver_symtbl.c

rpcserver_DEPENDENCIES=\
    rpcserver_symtbl.o $(TE_DEPENDENCIES) \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/librcfpch.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libloggerta.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libtools.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/librpc_types.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/librpcxdrta.a 

rpcserver_SOURCES=tarpc_server.c rpcserver.c common.c inet_pton.c inet_ntop.c

nodist_rpcserver_SOURCES=rpc_unsupported.c rpc_supported.h
rpcserver32_SOURCES=
rpcserver64_SOURCES=

rpcserver_LDADD=rpcserver_symtbl.o $(TE_LDADD) -lrpcxdrta \
                -lrpc_types -lrcfpch -lloggerta -ltools -lrpc \
                -lws2_32 -lmswsock -lwsock32 -liphlpapi 

rpcserver_symtbl.c: tarpc_server.o common.o $(RPCSERVER_SYMTBL_ADD)
	$(srcdir)/../../engine/builder/te_generate_symtbl "${NM}" $@ $^ 

# Native build

RPCSERVER_LIBS="librcfpch.a libloggerta.a libtools.a librpc.a librpc_types.a \
                ws2_32.lib mswsock.lib wsock32.lib iphlpapi.lib user32.lib"

RPCSERVER_SRC= $(rpcserver_SOURCES) \
               rpcserver_symtbl.c rpc_unsupported.c \
               ${srcdir}/../../lib/rpcxdr/rpc_xdr.c \
               ../../lib/@TE_PLATFORM@_rpcxdr/tarpc_xdr.c \
               $(RPCSERVER_SRC_ADD)

rpcserver32$(EXEEXT): $(RPCSERVER_SRC) $(rpcserver_DEPENDENCIES)
	TE_INSTALL=$(DESTDIR)/$(prefix)/.. \
        $(srcdir)/../../engine/builder/te_build_win_app $(srcdir) \
        $(TE_PLATFORM) windows $@ \"$(RPCSERVER_LIBS)\" $^
        
rpcserver64$(EXEEXT): $(RPCSERVER_SRC) $(rpcserver_DEPENDENCIES)
	TE_INSTALL=$(DESTDIR)/$(prefix)/.. \
        $(srcdir)/../../engine/builder/te_build_win_app $(srcdir) \
        $(TE_PLATFORM) windows_64 $@ \"$(RPCSERVER_LIBS)\" $^

install-data-hook:
	if test -e rpcserver32$(EXEEXT) -a ! -s rpcserver32$(EXEEXT) \
                -a -n "$$TE_WIN32_BUILD_HOST" ; then \
            rm rpcserver* ; $(MAKE) install ; \
        fi
	if test ! -s $(DESTDIR)/$(tadir)/rpcserver32$(EXEEXT) ; then \
            rm -f $(DESTDIR)/$(tadir)/rpcserver32$(EXEEXT) ; \
        fi
	if test ! -s $(DESTDIR)/$(tadir)/rpcserver64$(EXEEXT) ; then \
            rm -f $(DESTDIR)/$(tadir)/rpcserver64$(EXEEXT) ; \
        fi
	if test -s $(DESTDIR)/$(tadir)/rpcserver32$(EXEEXT) ; then \
            mv $(DESTDIR)/$(tadir)/rpcserver32$(EXEEXT) \
               $(DESTDIR)/$(tadir)/rpcserver$(EXEEXT) ; \
        fi

clean-local:
	rm -f *.exe *.tgz

distclean-local:
	rm -f *.exe *.tgz

else

# Built-in RPC server

if CFG_ISCSI

LDADD+=-liscsi_initiator_conf -lsetupapi

AM_CPPFLAGS+=-DWITH_ISCSI

ta_DEPENDENCIES += $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libiscsi_initiator_conf.a

endif

ta_PROGRAMS=ta

endif

else

# No RPC

ta_PROGRAMS=ta

LDADD=symtbl.o \
-lrcfpch -lrpcxdr -lrpc_types -l$(TAD) $(TE_LDADD) -lconf_oid -lloggerta \
-lpthread -lws2_32 -lmswsock -lwsock32 -liphlpapi -lrpc -ltools \
-lcygwin -lexpat 

endif

# TA symbol table

symtbl.c: $(SYMTBL_OBJECTS) $(SYMTBL_LIBS) $(TE_DEPENDENCIES)
	$(srcdir)/../../engine/builder/te_generate_symtbl "${NM}" $@ $^ 

