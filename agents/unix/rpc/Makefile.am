
ACLOCAL_AMFLAGS = -I $(TE_M4)

AM_CPPFLAGS = -I$(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/include $(TE_CPPFLAGS) \
              -I$(srcdir)/.. $(TE_CPPFLAGS_VAR)

AM_CFLAGS = -fno-strict-aliasing $(TE_CFLAGS) $(TE_CFLAGS_VAR)

noinst_HEADERS = tarpc_server.h
        
noinst_LIBRARIES=libtarpc.a

libtarpc_a_SOURCES=

if CFG_RCF_RPC

libtarpc_a_SOURCES += tarpc_server.c 
libtarpc_a_LIBADD=rpc_unsupported.o

BUILT_SOURCES = rpc_unsupported.c rpc_supported.h

CLEANFILES = rpc_unsupported.c rpc_supported.h

rpc_unsupported.c: $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/include/tarpc.h \
                   rpc_supported.h \
                   $(srcdir)/../../../include/rpc_unsupported.m4
	$(srcdir)/../../../engine/builder/te_rpc_unsupported $< 
	mv rpc_unsupported.c rpc_unsupported.c.tmp
	echo '#include "te_config.h"' > rpc_unsupported.c
	echo '#include "../config.h"' >> rpc_unsupported.c
	cat rpc_unsupported.c.tmp >> rpc_unsupported.c
	rm rpc_unsupported.c.tmp
        
rpc_supported.h: tarpc_server.o @TE_DEPENDENCIES@
	NM_FORMAT=@NM_FORMAT@ $(srcdir)/../../../engine/builder/te_rpc_supported \
		  "${NM}" $^

endif
