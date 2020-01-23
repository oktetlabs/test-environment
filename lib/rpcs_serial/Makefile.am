
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = -I$(DESTDIR)/$(includedir) \
			  @TE_CPPFLAGS@ \
			  $(TIRPC_CFLAGS) \
			  $(TE_CPPFLAGS_VAR)

AM_CFLAGS = @TE_CFLAGS@ $(TE_CFLAGS_VAR)

EXTRA_DIST = serial.x.m4

lib_LIBRARIES = librpcs_serial.a

librpcs_serial_a_SOURCES = \
	serial.c

install-exec-hook:
	for i in ${lib_LIBRARIES} ; do \
	    touch -c -r $$i $(DESTDIR)/$(prefix)/lib/$$i ; \
	done

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install
