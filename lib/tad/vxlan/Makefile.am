
AM_CPPFLAGS = -I$(top_srcdir) @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) \
              $(TE_CPPFLAGS_VAR)
AM_CFLAGS = @TE_CFLAGS@ $(TE_CFLAGS_VAR)

noinst_HEADERS = tad_vxlan_impl.h

noinst_LIBRARIES = libtad_vxlan.a

libtad_vxlan_a_SOURCES = \
	tad_vxlan_layer.c \
	tad_vxlan_csap.c
