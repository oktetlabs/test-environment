
tetestdir=$(prefix)/@PACKAGE_NAME@/$(subdir)

dist_tetest_DATA = package.xml


AM_CPPFLAGS = -I$(DESTDIR)/$(prefix)/include 
AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 


tetest_PROGRAMS = rpctest


rpctest_SOURCES = rpctest.c 

rpctest_LDADD = -ltapi_rpc -ltapi -lrcfrpc -lconfapi -lconf_oid \
                -lrcfapi -llogger_ten -lipc -lpthread

rpctest_DEPENDENCIES = ${TE_INSTALL}/@host@/lib/librcfapi.a \
                       ${TE_INSTALL}/@host@/lib/librcfrpc.a \
                       ${TE_INSTALL}/@host@/lib/libconfapi.a \
                       ${TE_INSTALL}/@host@/lib/libtapi_rpc.a \
                       ${TE_INSTALL}/@host@/lib/liblogger_ten.a

