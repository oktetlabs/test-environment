
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(myincludedir) -I$(DESTDIR)/$(includedir) $(TE_CPPFLAGS_VAR)
AM_CFLAGS = @TE_CFLAGS@ $(TE_CFLAGS_VAR)
AM_LDFLAGS = @TE_LDFLAGS@

myincludedir = $(includedir)/@include_subdir@

myinclude_HEADERS=agentlib.h agentlib_config.h agentlib_defs.h telephony.h dahdi_user.h

lib_LIBRARIES=lib@with_name@.a

lib@with_name@_a_SOURCES=\
    process.c \
    ftp.c net.c \
    l4_port.c \
    telephony.c \
    power_sw.c \
    symbols.c \
    unistd.c \
    keys.c

install-exec-hook:
	touch -c -r lib@with_name@.a $(DESTDIR)/$(libdir)/lib@with_name@.*

# agentlib_config.h is patched to have autoconf-generated macros prefixed, so
# that the file could be included into other libraries without causing macro redefinitions
install-data-hook:
	sed 's/#define PACKAGE/#define AGENTLIB_PACKAGE/; s/#define VERSION /#define AGENTLIB_VERSION /' \
		$(builddir)/agentlib_config.h >$(DESTDIR)/$(myincludedir)/agentlib_config.h
	touch -c -r $(builddir)/agentlib_config.h $(DESTDIR)/$(myincludedir)/agentlib_config.h
	for i in $(myinclude_HEADERS) ; do \
		if test -f $(srcdir)/$$i; then touch -c -r $(srcdir)/$$i $(DESTDIR)/$(myincludedir)/$$i ; fi; \
	done

all: install
