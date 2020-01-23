
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = -I$(DESTDIR)/$(includedir) \
              -I$(TE_BASE)/agents/@TA@/ \
              -I$(TE_BASE)/agents/@TA@/rpc \
              @TE_CPPFLAGS@ \
              $(TIRPC_CFLAGS) \
              $(TE_CPPFLAGS_VAR) \
              -D'INCLUDE(x)='

AM_CFLAGS = @TE_CFLAGS@ $(TE_CFLAGS_VAR)

AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(libdir)


EXTRA_DIST = job.x.m4

lib_LIBRARIES = librpcs_job.a

librpcs_job_a_SOURCES = \
	job.c


install-exec-hook:
	for i in ${lib_LIBRARIES} ; do \
	    touch -c -r $$i $(DESTDIR)/$(prefix)/lib/$$i ; \
	done

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install
