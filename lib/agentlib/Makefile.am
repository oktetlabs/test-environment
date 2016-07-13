
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) $(TE_CPPFLAGS_VAR)
AM_CFLAGS = @TE_CFLAGS@ $(TE_CFLAGS_VAR)
AM_LDFLAGS = @TE_LDFLAGS@

include_HEADERS=agentlib.h agentlib_config.h telephony.h dahdi_user.h

lib_LIBRARIES=libagentlib.a

libagentlib_a_SOURCES=process.c ftp.c net.c telephony.c power_sw.c

install-exec-hook:
	touch -c -r libagentlib.a $(DESTDIR)/$(libdir)/libagentlib.*

# agentlib_config.h is patched to have autoconf-generated macros prefixed, so
# that the file could be included into other libraries without causing macro redefinitions
install-data-hook:
	sed 's/#define PACKAGE/#define AGENTLIB_PACKAGE/; s/#define VERSION /#define AGENTLIB_VERSION /' \
		$(builddir)/agentlib_config.h >$(DESTDIR)/$(includedir)/agentlib_config.h
	touch -c -r $(builddir)/agentlib_config.h $(DESTDIR)/$(includedir)/agentlib_config.h
	for i in $(include_HEADERS) ; do \
		if test -f $(srcdir)/$$i; then touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; fi; \
	done

all: install

