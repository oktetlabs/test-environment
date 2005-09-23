
AM_CPPFLAGS = @TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include 
AM_LDFLAGS = @TE_LDFLAGS@ 


include_HEADERS = logger_ta.h logger_ta_internal.h logger_ta_lock.h \
                  logfork.h


lib_LIBRARIES = libloggerta.a

libloggerta_a_SOURCES = logger_ta.c logfork.c


install-exec-hook:
	touch -c -r libloggerta.a ${prefix}/lib/libloggerta.a

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install
