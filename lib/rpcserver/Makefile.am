ACLOCAL_AMFLAGS = -I $(TE_M4)

AM_CPPFLAGS = -I$(DESTDIR)/$(myincludedir) -I$(DESTDIR)/$(includedir)/@with_agentlib@ \
			  -I$(DESTDIR)/$(includedir) $(TE_CPPFLAGS) \
              $(EXTRA_CPPFLAGS) $(TE_CPPFLAGS_VAR)

AM_CFLAGS = -fno-strict-aliasing $(TE_CFLAGS) $(TE_CFLAGS_VAR)

myincludedir = $(includedir)/@include_subdir@

myinclude_HEADERS = rpc_server.h rcf_pch_mem.h rpc_server_config.h iomux.h rpcs_msghdr.h

lib_LTLIBRARIES=lib@with_name@.la

lib@with_name@_la_SOURCES=rpc_server.c rcf_pch_mem.c tarpc_imp.c iomux.c rpcs_msghdr.c

install-exec-hook:
	touch -c -r lib@with_name@.la $(DESTDIR)/$(libdir)/lib@with_name@.*

# rpc_server_config.h is patched to have autoconf-generated macros prefixed, so
# that the file could be included into other libraries without causing macro redefinitions
install-data-hook:
	sed 's/#define PACKAGE/#define RPCSERVER_PACKAGE/; s/#define VERSION /#define RPCSERVER_VERSION /' \
		$(builddir)/rpc_server_config.h >$(DESTDIR)/$(myincludedir)/rpc_server_config.h
	touch -c -r $(builddir)/rpc_server_config.h $(DESTDIR)/$(myincludedir)/rpc_serer_config.h
	for i in $(myinclude_HEADERS) ; do \
		if test -f $(srcdir)/$$i; then touch -c -r $(srcdir)/$$i $(DESTDIR)/$(myincludedir)/$$i ; fi; \
	done

