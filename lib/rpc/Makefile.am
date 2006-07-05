
rpcdir=$(includedir)/rpc

rpc_HEADERS=rpc.h types.h xdr.h
noinst_HEADERS=internal.h

lib_LIBRARIES=librpc.a

librpc_a_SOURCES=\
xdr.c \
xdr_array.c \
xdr_intXX_t.c \
xdr_mem.c \
xdr_ref.c \
xdr_sizeof.c

if WINDOWS

librpc.a: $(librpc_a_SOURCES) 
	TE_INSTALL=$(DESTDIR)/$(prefix)/.. \
        $(srcdir)/../../engine/builder/te_build_win_lib $(srcdir) \
        $(TE_PLATFORM) $@ $(DESTDIR)/$(prefix) $(librpc_a_SOURCES)

install-exec-hook:
	if test -e $(lib_LIBRARIES) -a ! -s $(lib_LIBRARIES) \
                -a -n "$$TE_WIN32_BUILD_HOST" ; then \
            rm $(lib_LIBRARIES) ; $(MAKE) install ; \
        fi

else

AM_CPPFLAGS=-I.

install-exec-hook:
	touch -c -r libloggerta.* $(DESTDIR)/$(libdir)/libloggerta.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install

endif

