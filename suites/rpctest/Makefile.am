
tetestdir=$(prefix)/@PACKAGE_NAME@/$(subdir)

dist_tetest_DATA = package.xml

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) -g
AM_CFLAGS = @TE_CFLAGS@ 
AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(libdir)

MY_LDADD=-ltapi_rpc -lrpc_types -ltapi_env -ltapi -lrcfrpc \
         -lconfapi -lconf_oid -lrcfapi -lrpcxdr -llogger_ten -lipc \
         -llog_bufs -ltools -lexpat -lpthread

MY_DEPS= \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libtapi_rpc.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libtapi.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libtapi_env.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/librpc_types.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/librcfrpc.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libconfapi.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libconf_oid.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/librcfapi.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/librpcxdr.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/liblogger_ten.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libipc.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/liblog_bufs.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libtools.a

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
