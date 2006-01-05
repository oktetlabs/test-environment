
AM_CPPFLAGS = -I$(top_srcdir) @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@ 

noinst_HEADERS=tad_dhcp_impl.h

noinst_LIBRARIES=libtad_dhcp.a

libtad_dhcp_a_SOURCES=tad_dhcp_stack.c tad_dhcp_layer.c tad_dhcp_csap.c 


install-exec-hook:
	touch -c -r libtad_dhcp.a $(DESTDIR)/$(libdir)/libtad_dhcp.a

all: install

