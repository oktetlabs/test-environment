
AM_CPPFLAGS = -I$(DESTDIR)/$(prefix)/include \
              -I$(DESTDIR)/$(prefix)/../include

# TE_CFLAGS are already included in CFLAGS in configure.ac
AM_CFLAGS =

AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 


include_HEADERS = logger_ten.h logger_ten_int.h


lib_LIBRARIES = liblogger_ten.a

liblogger_ten_a_SOURCES = logger_ten.c


install-exec-hook:
	touch -c -r liblogger_ten.a ${prefix}/lib/liblogger_ten.a

install-data-hook:
	for i in $(include_HEADERS) ; do \
		touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install
