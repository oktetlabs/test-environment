
AUTOMAKE_OPTIONS = foreign 

AM_CPPFLAGS=-I$(DESTDIR)/$(prefix)/include \
            -I$(DESTDIR)/$(prefix)/../include 

AM_CFLAGS=

AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 


noinst_HEADERS=tad_dhcp_impl.h

lib_LIBRARIES=libtad_dhcp.a

libtad_dhcp_a_SOURCES=tad_dhcp_stack.c tad_dhcp_layer.c tad_dhcp_csap.c 


install-exec-hook:
	touch -c -r libtad_dhcp.a $(DESTDIR)/$(prefix)/lib/libtad_dhcp.a

all: install

