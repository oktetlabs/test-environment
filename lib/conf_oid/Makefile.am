
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) $(TE_CPPFLAGS_VAR)
AM_CFLAGS = @TE_CFLAGS@ $(TE_CFLAGS_VAR)

include_HEADERS=conf_oid.h \
    conf_object.h \
    conf_ip.h \
    conf_ip_rule.h

lib_LTLIBRARIES=libconf_oid.la

libconf_oid_la_SOURCES=conf_oid.c \
    conf_object.c \
    conf_ip_rule.c

install-exec-hook:
	touch -c -r libconf_oid.la $(DESTDIR)/$(libdir)/libconf_oid.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
		touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install

