
AUTOMAKE_OPTIONS = foreign 

AM_CPPFLAGS=-I$(DESTDIR)/$(prefix)/include \
            -I$(DESTDIR)/$(prefix)/../include \
            -DCMU_COMPATIBLE -DUCD_COMPATIBLE

AM_CFLAGS=@NET_SNMP_CFLAGS@

AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 


# Tests (incorrect)
#VPATH=@srcdir@:@srcdir@/tests
#TESTS=cdb01 cdb02
#check_PROGRAMS=cdb01 cdb02
#extra_DIST=tests/cdb01.c tests/cdb02.c
#LDADD=-lasn @NET_SNMP_LIBS@
#LDFLAGS=-static

noinst_HEADERS=tad_snmp_impl.h

noinst_LIBRARIES=libtad_snmp.a


libtad_snmp_a_SOURCES=tad_snmp_stack.c tad_snmp_layer.c tad_snmp_csap.c 

#libtad_snmp_a_LIBADD= @NET_SNMP_LIBS@


install-exec-hook:
	touch -c -r libtad_snmp.a $(DESTDIR)/$(prefix)/lib/libtad_snmp.a

all: install

