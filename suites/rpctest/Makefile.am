
tetestdir=$(prefix)/@PACKAGE_NAME@/$(subdir)

dist_tetest_DATA = package.xml

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) -g
AM_CFLAGS = @TE_CFLAGS@ 
AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(libdir)

MY_LDADD=-ltapi_rpc -lrpc_types -ltapi_env -ltapi -lrcfrpc \
         -lconfapi -lconf_oid \
         -lrcfapi -lrpcxdr -llogger_ten -lipc -ltools -lexpat -lpthread -lfl

MY_DEPS= \
    ${TE_INSTALL}/$(TE_PLATFORM)/lib/libtapi_rpc.a \
    ${TE_INSTALL}/$(TE_PLATFORM)/lib/libtapi.a \
    ${TE_INSTALL}/$(TE_PLATFORM)/lib/libtapi_env.a \
    ${TE_INSTALL}/$(TE_PLATFORM)/lib/librpc_types.a \
    ${TE_INSTALL}/$(TE_PLATFORM)/lib/librcfrpc.a \
    ${TE_INSTALL}/$(TE_PLATFORM)/lib/libconfapi.a \
    ${TE_INSTALL}/$(TE_PLATFORM)/lib/libconf_oid.a \
    ${TE_INSTALL}/$(TE_PLATFORM)/lib/librcfapi.a \
    ${TE_INSTALL}/$(TE_PLATFORM)/lib/librpcxdr.a \
    ${TE_INSTALL}/$(TE_PLATFORM)/lib/liblogger_ten.a \
    ${TE_INSTALL}/$(TE_PLATFORM)/lib/libipc.a \
    ${TE_INSTALL}/$(TE_PLATFORM)/lib/libtools.a

noinst_HEADERS=rpc_suite.h         

tetest_PROGRAMS = prologue rpctest rs_threads_sr


prologue_SOURCES = prologue.c 
prologue_LDADD = $(MY_LDADD)
prologue_DEPENDENCIES = $(MY_DEPS)

rpctest_SOURCES = rpctest.c 
rpctest_LDADD = $(MY_LDADD)
rpctest_DEPENDENCIES = $(MY_DEPS)

rs_threads_sr_SOURCES = rs_threads_sr.c 
rs_threads_sr_LDADD = $(MY_LDADD)
rs_threads_sr_DEPENDENCIES = $(MY_DEPS)
