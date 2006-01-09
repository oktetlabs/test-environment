
AM_CPPFLAGS = -I$(top_srcdir) @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@ 

noinst_HEADERS = tad_atm_impl.h

noinst_LIBRARIES = libtad_atm.a

libtad_atm_a_SOURCES = tad_atm_csap.c tad_atm_layer.c tad_aal5_layer.c


install-exec-hook:
	touch -c -r libtad_atm.a $(DESTDIR)/$(libdir)/libtad_atm.a

all: install
