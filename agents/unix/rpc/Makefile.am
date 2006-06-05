
ACLOCAL_AMFLAGS = -I $(TE_M4)

AM_CPPFLAGS = -I$(DESTDIR)/$(prefix)/../@host@/include $(TE_CPPFLAGS) \
              -include te_config.h -include ../config.h -I$(srcdir)/..
              
AM_CFLAGS = -fno-strict-aliasing $(TE_CFLAGS)

noinst_HEADERS = tarpc_server.h
        
noinst_LIBRARIES=libtarpc.a

libtarpc_a_SOURCES=

if CFG_RCF_RPC

libtarpc_a_SOURCES += tarpc_server.c 
libtarpc_a_LIBADD=rpc_unsupported.o

BUILT_SOURCES = rpc_unsupported.c rpc_supported.h

CLEANFILES = rpc_unsupported.c rpc_supported.h

rpc_unsupported.c: $(DESTDIR)/$(prefix)/../@host@/include/tarpc.h \
                   rpc_supported.h \
                   $(srcdir)/../../../include/rpc_unsupported.m4
	$(srcdir)/../../../engine/builder/te_rpc_unsupported $< 
        
rpc_supported.h: tarpc_server.o @TE_DEPENDENCIES@
	$(srcdir)/../../../engine/builder/te_rpc_supported "${NM}" $^

endif
