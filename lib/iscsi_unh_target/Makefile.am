
SUBDIRS = common security target

ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) \
              -I$(srcdir)/common -I$(srcdir)/target
AM_CFLAGS = @TE_CFLAGS@ -pthread
AM_LDFLAGS = @TE_LDFLAGS@


noinst_PROGRAMS = unh_targetd


unh_targetd_SOURCES = main.c

unh_targetd_LDADD = target/libiscsi_target.a security/libiscsi_security.a \
                    common/libiscsi_common.a


lib_LIBRARIES = libiscsi_unh_target.a

libiscsi_unh_target_a_SOURCES =


install-exec-hook:
	touch -c -r libiscsi_unh_target.a \
		$(DESTDIR)/$(libdir)/libiscsi_unh_target.a

install-exec-local:
	for mod in common security target; do \
		for obj in $$mod/*.o; do \
			$(AR) $(ARFLAGS) libiscsi_unh_target.a $$obj; \
		done; \
	done
	$(RANLIB) libiscsi_unh_target.a


all: install
