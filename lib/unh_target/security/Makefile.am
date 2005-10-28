
AM_CPPFLAGS = -I$(DESTDIR)/$(includedir) -I$(srcdir)/../common


pkginclude_HEADERS = chap.h misc_func.h 

noinst_HEADERS = sha1.h md5.h bigint.h srp.h


noinst_LIBRARIES = libiscsi_security.a

libiscsi_security_a_SOURCES = chap.c srp.c misc_func.c \
                              sha1.c md5.c bigint.c


install-data-hook:
	for i in $(pkginclude_HEADERS) ; do \
		touch -c -r $(srcdir)/$$i $(DESTDIR)/$(pkgincludedir)/$$i ; \
	done
