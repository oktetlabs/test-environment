
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) $(TE_CPPFLAGS_VAR)

AM_CFLAGS = @TE_CFLAGS@ $(TE_CFLAGS_VAR) $(GLIB_CFLAGS) $(LIBXML_CFLAGS) \
            $(LIBSOUP_CFLAGS) $(GSSDP_CFLAGS) $(GUPNP_CFLAGS) $(GUPNPAV_CFLAGS) \
            $(JANSSON_CFLAGS)

AM_LDFLAGS = @TE_LDFLAGS@

LIBS = @LIBS@

noinst_HEADERS = te_upnp_cp.h

include_HEADERS = conf_upnp_cp.h

lib_LIBRARIES = lib@with_name@.a

lib@with_name@_a_SOURCES = tarpc_upnp_cp.c conf_upnp_cp.c te_upnp_cp.c

mypkgdir = $(libdir)/pkgconfig
mypkg_DATA = te-@with_name@.pc
CLEANFILES = te-@with_name@.pc

te-@with_name@.pc:
	{ \
	    echo "Name: te-$(with_name)" ; \
	    echo "Description: UPnP Control Point for TE" ; \
	    echo "Version: $(PACKAGE_VERSION)" ; \
	    echo "Requires:" ; \
	    echo "Libs: @TA_UPNP_CP_LIBS@" ; \
	    echo "Cflags: " ; \
	} > $@

install-exec-hook:
	touch -c -r lib@with_name@.a $(DESTDIR)/$(libdir)/lib@with_name@.a

install-data-hook:
	sed 's/#define PACKAGE/#define UPNP_CP_PACKAGE/; s/#define VERSION /#define UPNP_CP_VERSION /' \
		$(builddir)/upnp_cp_config.h >$(DESTDIR)/$(includedir)/upnp_cp_config.h
	touch -c -r $(builddir)/upnp_cp_config.h $(DESTDIR)/$(includedir)/upnp_cp_config.h
	touch -c -r te-@with_name@.pc $(mybindir)/te-@with_name@.pc
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install
