
tetestdir=$(prefix)/@PACKAGE_NAME@/$(subdir)

dist_tetest_DATA = package.xml

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) -g
AM_CFLAGS = @TE_CFLAGS@ 
AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(libdir)

MY_LDADD = -ltapi_rpc -lrpc_types -ltapi_env -ltapi -lasn -lndn -lrcfrpc \
           -lconfapi -lconf_oid -lrcfapi -lrpcxdr -llogger_ten -lipc \
           -llog_bufs -ltools -lpthread -ltapi_acse -lcwmp_data

MY_DEPS = \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libtapi_rpc.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libtapi.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libasn.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libndn.a \
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

tetest_PROGRAMS = prologue epilogue acse download set_param change_wan \
                    get_param hold_req add_object

prologue_SOURCES = prologue.c
prologue_LDADD = $(MY_LDADD)
prologue_DEPENDENCIES = $(MY_DEPS)

acse_SOURCES = acse.c
acse_LDADD = $(MY_LDADD)
acse_DEPENDENCIES = $(MY_DEPS)

set_param_SOURCES = set_param.c
set_param_LDADD = $(MY_LDADD)
set_param_DEPENDENCIES = $(MY_DEPS)

get_param_SOURCES = get_param.c
get_param_LDADD = $(MY_LDADD)
get_param_DEPENDENCIES = $(MY_DEPS)

download_SOURCES = download.c
download_LDADD = $(MY_LDADD)
download_DEPENDENCIES = $(MY_DEPS)

change_wan_SOURCES = change_wan.c
change_wan_LDADD = $(MY_LDADD)
change_wan_DEPENDENCIES = $(MY_DEPS)

hold_req_SOURCES = hold_req.c
hold_req_LDADD = $(MY_LDADD)
hold_req_DEPENDENCIES = $(MY_DEPS)

add_object_SOURCES = add_object.c
add_object_LDADD = $(MY_LDADD)
add_object_DEPENDENCIES = $(MY_DEPS)

epilogue_SOURCES = epilogue.c
epilogue_LDADD = $(MY_LDADD)
epilogue_DEPENDENCIES = $(MY_DEPS)



