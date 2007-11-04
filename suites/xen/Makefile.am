
tetestdir=$(prefix)/@PACKAGE_NAME@/$(subdir)

dist_tetest_DATA = package.xml

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) -g
AM_CFLAGS = @TE_CFLAGS@ 
AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(libdir)

MY_LDADD = -ltapi_rpc -lrpc_types -ltapi_env -ltapi -lrcfrpc \
           -lconfapi -lconf_oid -lrcfapi -lrpcxdr -llogger_ten -lipc \
           -llog_bufs -ltools -lpthread -ltapi_tad -lndn -lasn

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
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libtapi_tad.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libndn.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libasn.a

noinst_HEADERS = xen_suite.h xen.h

tetest_PROGRAMS = prologue xen_dhcp xen_path xen_create_destroy xen_startup xen_saved xen_migrate xen_saved_migrate xen_rcf


prologue_SOURCES = prologue.c
prologue_LDADD = $(MY_LDADD)
prologue_DEPENDENCIES = $(MY_DEPS)

xen_dhcp_SOURCES = xen_dhcp.c
xen_dhcp_LDADD = $(MY_LDADD)
xen_dhcp_DEPENDENCIES = $(MY_DEPS)

xen_path_SOURCES = xen_path.c
xen_path_LDADD = $(MY_LDADD)
xen_path_DEPENDENCIES = $(MY_DEPS)

xen_create_destroy_SOURCES = xen_create_destroy.c
xen_create_destroy_LDADD = $(MY_LDADD)
xen_create_destroy_DEPENDENCIES = $(MY_DEPS)

xen_startup_SOURCES = xen_startup.c
xen_startup_LDADD = $(MY_LDADD)
xen_startup_DEPENDENCIES = $(MY_DEPS)

xen_saved_SOURCES = xen_saved.c
xen_saved_LDADD = $(MY_LDADD)
xen_saved_DEPENDENCIES = $(MY_DEPS)

xen_migrate_SOURCES = xen_migrate.c
xen_migrate_LDADD = $(MY_LDADD)
xen_migrate_DEPENDENCIES = $(MY_DEPS)

xen_saved_migrate_SOURCES = xen_saved_migrate.c
xen_saved_migrate_LDADD = $(MY_LDADD)
xen_saved_migrate_DEPENDENCIES = $(MY_DEPS)

xen_rcf_SOURCES = xen_rcf.c
xen_rcf_LDADD = $(MY_LDADD)
xen_rcf_DEPENDENCIES = $(MY_DEPS)
