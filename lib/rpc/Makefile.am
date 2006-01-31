
rpcdir=$(includedir)/rpc

rpc_HEADERS=rpc.h types.h xdr.h
noinst_HEADERS=internal.h

RPC_SOURCES=\
xdr.c \
xdr_array.c \
xdr_intXX_t.c \
xdr_mem.c \
xdr_ref.c \
xdr_sizeof.c

if WINDOWS

lib_LIBRARIES=librpc.lib

librpc_lib_SOURCES=$(RPC_SOURCES)

librpc.lib: $(RPC_SOURCES) 
	$(srcdir)/../../engine/builder/te_build_win_lib $(srcdir) @host@ $@ \
        $(DESTDIR)/$(prefix) $(RPC_SOURCES)

else

AM_CPPFLAGS=-I.

lib_LIBRARIES=librpc.a

librpc_a_SOURCES=$(RPC_SOURCES)

endif

