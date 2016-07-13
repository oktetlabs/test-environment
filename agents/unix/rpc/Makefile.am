ACLOCAL_AMFLAGS = -I $(TE_M4)

AM_CPPFLAGS = -I$(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/include $(TE_CPPFLAGS) \
              -I$(srcdir)/.. $(TE_CPPFLAGS_VAR)

AM_CFLAGS = -fno-strict-aliasing $(TE_CFLAGS) $(TE_CFLAGS_VAR)

noinst_HEADERS = tarpc_server.h

noinst_LIBRARIES=libtarpc.a

libtarpc_a_SOURCES=

if CFG_RCF_RPC

libtarpc_a_SOURCES += tarpc_server.c

CLEANFILES = $(BUILT_SOURCES)

endif
