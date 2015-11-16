
AM_CPPFLAGS = -DCMU_COMPATIBLE -DUCD_COMPATIBLE -I$(top_srcdir) \
               @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) \
               $(TE_CPPFLAGS_VAR)
AM_CFLAGS = @TE_CFLAGS@ @NET_SNMP_CFLAGS@ $(TE_CFLAGS_VAR)

noinst_HEADERS = tad_snmp_impl.h

noinst_LIBRARIES = libtad_snmp.a

libtad_snmp_a_SOURCES = tad_snmp_stack.c tad_snmp_layer.c tad_snmp_csap.c 
