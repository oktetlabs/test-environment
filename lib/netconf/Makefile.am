
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) $(TE_CPPFLAGS_VAR)
AM_CFLAGS = @TE_CFLAGS@ $(TE_CFLAGS_VAR)

lib_LIBRARIES = libnetconf.a

include_HEADERS = netconf.h netconf_internal.h

libnetconf_a_SOURCES = netconf.c \
	link.c \
	net_addr.c \
	route.c \
	neigh.c \
	rule.c

install-exec-hook:
	touch -c -r libnetconf.a $(DESTDIR)/$(libdir)/libnetconf.a

install-data-hook:
	for i in $(include_HEADERS) ; do \
		touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install
