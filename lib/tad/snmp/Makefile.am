
AM_CPPFLAGS=@TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include \
            -DCMU_COMPATIBLE -DUCD_COMPATIBLE -I$(top_srcdir)

AM_CFLAGS=@NET_SNMP_CFLAGS@

AM_LDFLAGS=@TE_LDFLAGS@ 


noinst_HEADERS=tad_snmp_impl.h

noinst_LIBRARIES=libtad_snmp.a


libtad_snmp_a_SOURCES=tad_snmp_stack.c tad_snmp_layer.c tad_snmp_csap.c 

#libtad_snmp_a_LIBADD= @NET_SNMP_LIBS@


install-exec-hook:
	touch -c -r libtad_snmp.a $(DESTDIR)/$(prefix)/lib/libtad_snmp.a

all: install

