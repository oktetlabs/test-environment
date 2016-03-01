
#EXTRA_DIST = package.dox

tetestdir=$(prefix)/@PACKAGE_NAME@/$(subdir)

dist_tetest_DATA = package.xml

tetest_DATA = tests-info.xml

tests-info.xml: $(srcdir)/*.c
	@TE_PATH@/te_tests_info.sh $(srcdir) > tests-info.xml

CLEANFILES = tests-info.xml


AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@
AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(libdir)

MY_LDADD=-ltapi_rpc -lrpc_types -ltapi -lasn -lndn -lrcfrpc \
         -lconfapi -lconf_oid -lrcfapi -lrpcxdr -llogger_ten -lipc \
         -llog_bufs -ltools -lexpat -lpthread -ltapi_env -ltapi_upnp

MY_DEPS = \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/librcfapi.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/liblogger_ten.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libipc.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libtapi.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libconfapi.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libconf_types.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libtapi_env.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libtapi_upnp.a

tetest_PROGRAMS = \
	prologue \
	epilogue \
	get_devices \
	get_services \
	invoke_action \
	get_content


prologue_SOURCES = prologue.c
prologue_LDADD = $(MY_LDADD)
prologue_DEPENDENCIES = $(MY_DEPS)

epilogue_SOURCES = epilogue.c
epilogue_LDADD = $(MY_LDADD)
epilogue_DEPENDENCIES = $(MY_DEPS)

get_devices_SOURCES = get_devices.c
get_devices_LDADD = $(MY_LDADD)
get_devices_DEPENDENCIES = $(MY_DEPS)

get_services_SOURCES = get_services.c
get_services_LDADD = $(MY_LDADD)
get_services_DEPENDENCIES = $(MY_DEPS)

invoke_action_SOURCES = invoke_action.c
invoke_action_LDADD = $(MY_LDADD)
invoke_action_DEPENDENCIES = $(MY_DEPS)

get_content_SOURCES = get_content.c
get_content_LDADD = $(MY_LDADD)
get_content_DEPENDENCIES = $(MY_DEPS)
