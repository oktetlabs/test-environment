
AUTOMAKE_OPTIONS = foreign 

AM_CFLAGS=@TE_CFLAGS@ @NET_SNMP_CFLAGS@ -I$(DESTDIR)/$(prefix)/include \
          -I$(DESTDIR)/$(prefix)/../include \
          -DCMU_COMPATIBLE -DUCD_COMPATIBLE

AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 

# Tests (incorrect)
#VPATH=@srcdir@:@srcdir@/tests
#TESTS=cdb01 cdb02
#check_PROGRAMS=cdb01 cdb02
extra_DIST=tests/cdb01.c tests/cdb02.c

lib_LIBRARIES=libtad_snmp.a

libtad_snmp_a_SOURCES=tad_snmp_stack.c tad_snmp_layer.c tad_snmp_csap.c 

install-exec-hook:
	touch -c -r libtad_snmp.a $(DESTDIR)/$(prefix)/lib/libtad_snmp.a

LDADD=-lasn @NET_SNMP_LIBS@
LDFLAGS=-static


all: install

