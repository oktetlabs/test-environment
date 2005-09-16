
tetestdir=$(prefix)/@PACKAGE_NAME@/$(subdir)

dist_tetest_DATA = package.xml

AM_CPPFLAGS = -I$(DESTDIR)/$(prefix)/include -g
AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 

MY_LDADD=-ltapi_rpc -ltapi_env -ltapi -lrcfrpc -lconfapi -lconf_oid \
         -lrcfapi -lrpcxdr -llogger_ten -lipc -ltools -lexpat -lpthread -lfl

MY_DEPENDENCIES = \
    ${TE_INSTALL}/@host@/lib/libtapi_rpc.a \
    ${TE_INSTALL}/@host@/lib/libtapi.a \
    ${TE_INSTALL}/@host@/lib/libtapi_env.a \
    ${TE_INSTALL}/@host@/lib/librcfrpc.a \
    ${TE_INSTALL}/@host@/lib/libconfapi.a \
    ${TE_INSTALL}/@host@/lib/libconf_oid.a \
    ${TE_INSTALL}/@host@/lib/librcfapi.a \
    ${TE_INSTALL}/@host@/lib/librpcxdr.a \
    ${TE_INSTALL}/@host@/lib/liblogger_ten.a \
    ${TE_INSTALL}/@host@/lib/libipc.a \
    ${TE_INSTALL}/@host@/lib/libtools.a

noinst_HEADERS=rpc_suite.h         

tetest_PROGRAMS = prologue rpctest rs_threads_sr


prologue_SOURCES = prologue.c 
prologue_LDADD = $(MY_LDADD)
prologue_DEPENDENCIES = $(MY_DEPENDENCIES)

rpctest_SOURCES = rpctest.c 
rpctest_LDADD = $(MY_LDADD)
rpctest_DEPENDENCIES = $(MY_DEPENDENCIES)

rs_threads_sr_SOURCES = rs_threads_sr.c 
rs_threads_sr_LDADD = $(MY_LDADD)
rs_threads_sr_DEPENDENCIES = $(MY_DEPENDENCIES)
