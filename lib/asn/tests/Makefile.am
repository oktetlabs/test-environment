
AUTOMAKE_OPTIONS = foreign 

AM_CPPFLAGS=@TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include
AM_CFLAGS =
AM_LDFLAGS=@TE_LDFLAGS@ 



CFLAGS=@CFLAGS@ $(SNMP_F)

TESTS=parse01 parse04
#TESTS=ndn_set01 oid1 parse00 parse01 parse03 parse04 prn01 prn02 \
#val_set01 val_set02 val_set03 val_set04 write_ind1
check_PROGRAMS=$(TESTS)

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

noinst_HEADERS=test_types.h

# link flags for tests. 
LDADD=-lasn -lndn

