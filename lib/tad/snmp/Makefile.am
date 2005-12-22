
AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) \
              -DCMU_COMPATIBLE -DUCD_COMPATIBLE -I$(top_srcdir)
AM_CFLAGS = @TE_CFLAGS@ @NET_SNMP_CFLAGS@


noinst_HEADERS=tad_snmp_impl.h

noinst_LIBRARIES=libtad_snmp.a


libtad_snmp_a_SOURCES=tad_snmp_stack.c tad_snmp_layer.c tad_snmp_csap.c 


install-exec-hook:
	touch -c -r libtad_snmp.a $(DESTDIR)/$(libdir)/libtad_snmp.a

all: install

