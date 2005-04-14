
AUTOMAKE_OPTIONS = foreign 

AM_CPPFLAGS=@TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include
AM_CFLAGS =
AM_LDFLAGS=@TE_LDFLAGS@ 


lib_LIBRARIES=libasn.a

include_HEADERS=asn_usr.h asn_impl.h

libasn_a_SOURCES= asn_val.c asn_text.c 

CFLAGS=@CFLAGS@ $(SNMP_F)

# Tests (incorrect)
#
#VPATH = @srcdir@:@srcdir@/tests
#TESTS=ndn_set01 oid1 parse00 parse01 parse03 parse04 prn01 prn02 \
#val_set01 val_set02 val_set03 val_set04 write_ind1
#check_PROGRAMS=$(TESTS)

extra_DIST= \
tests/ndn_set01.c \
tests/oid1.c \
tests/parse00.c \
tests/parse01.c \
tests/parse03.c \
tests/parse04.c \
tests/prn01.c \
tests/prn02.c \
tests/val_set01.c \
tests/val_set02.c \
tests/val_set03.c \
tests/val_set04.c \
tests/write_ind1

noinst_HEADERS=tests/test_types.h

# link flags for tests. 
LDADD=-lasn -lndn

install-exec-hook:
	touch -c -r libasn.a $(DESTDIR)/$(prefix)/lib/libasn.a 

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install

