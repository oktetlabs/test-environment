
tetestdir=$(prefix)/@PACKAGE_NAME@/$(subdir)

dist_tetest_DATA = package.xml

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) -g
AM_CFLAGS = @TE_CFLAGS@ 
AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(libdir)

MY_LDADD = -ltapi_rpc -lrpc_types -ltapi_env -ltapi -lrcfrpc \
           -lconfapi -lconf_oid -lrcfapi -lrpcxdr -llogger_ten -lipc \
           -llog_bufs -ltools -lpthread -ltapi_acse -lcwmp_data

MY_DEPS = \
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
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libtools.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libtapi_acse.a

noinst_HEADERS = acse_suite.h

tetest_PROGRAMS = prologue acse epilogue

prologue_SOURCES = prologue.c
prologue_LDADD = $(MY_LDADD)
prologue_DEPENDENCIES = $(MY_DEPS)

acse_SOURCES = acse.c
acse_LDADD = $(MY_LDADD)
acse_DEPENDENCIES = $(MY_DEPS)

epilogue_SOURCES = epilogue.c
epilogue_LDADD = $(MY_LDADD)
epilogue_DEPENDENCIES = $(MY_DEPS)



