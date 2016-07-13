ACLOCAL_AMFLAGS = -I $(TE_M4)

AM_CPPFLAGS = -I$(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/include $(TE_CPPFLAGS) \
              $(TE_CPPFLAGS_VAR)

AM_CFLAGS = -fno-strict-aliasing $(TE_CFLAGS) $(TE_CFLAGS_VAR)

include_HEADERS = rpc_server.h rcf_pch_mem.h rpc_server_config.h

lib_LTLIBRARIES=librpcserver.la

librpcserver_la_SOURCES=rpc_server.c rcf_pch_mem.c aux_threads.c tarpc_imp.c

install-exec-hook:
	touch -c -r librpcserver.la $(DESTDIR)/$(libdir)/librpcserver.*

# rpc_server_config.h is patched to have autoconf-generated macros prefixed, so
# that the file could be included into other libraries without causing macro redefinitions
install-data-hook:
	sed 's/#define PACKAGE/#define RPCSERVER_PACKAGE/; s/#define VERSION /#define RPCSERVER_VERSION /' \
		$(builddir)/rpc_server_config.h >$(DESTDIR)/$(includedir)/rpc_server_config.h
	touch -c -r $(builddir)/rpc_server_config.h $(DESTDIR)/$(includedir)/rpc_serer_config.h
	for i in $(include_HEADERS) ; do \
		if test -f $(srcdir)/$$i; then touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; fi; \
	done

