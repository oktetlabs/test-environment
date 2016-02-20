
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) $(TE_CPPFLAGS_VAR)

AM_CFLAGS = @TE_CFLAGS@ $(TE_CFLAGS_VAR) $(GLIB_CFLAGS) $(LIBXML_CFLAGS) \
            $(LIBSOUP_CFLAGS) $(GSSDP_CFLAGS) $(GUPNP_CFLAGS) $(GUPNPAV_CFLAGS) \
            $(JANSSON_CFLAGS)

AM_LDFLAGS = -L$(DESTDIR)/$(libdir) $(GLIB_LIBS) $(LIBXML_LIBS) $(LIBSOUP_LIBS) \
             $(GSSDP_LIBS) $(GUPNP_LIBS) $(GUPNPAV_LIBS) $(JANSSON_LIBS)

lib_LTLIBRARIES=libtapi_upnp.la

include_HEADERS= \
	tapi_upnp.h \
	tapi_upnp_device_info.h \
	tapi_upnp_service_info.h \
	tapi_upnp_content_directory.h \
	tapi_upnp_resources.h \
	tapi_upnp_cp.h

libtapi_upnp_la_SOURCES= \
	tapi_upnp.c \
	tapi_upnp_device_info.c \
	tapi_upnp_service_info.c \
	tapi_upnp_content_directory.c \
	tapi_upnp_resources.c \
	tapi_upnp_cp.c


install-exec-hook:
	touch -c -r libtapi_upnp.la $(DESTDIR)/$(libdir)/libtapi_upnp.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
		touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install
