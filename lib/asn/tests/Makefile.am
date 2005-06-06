
AUTOMAKE_OPTIONS = foreign 

AM_CPPFLAGS=@TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include
AM_CFLAGS =
AM_LDFLAGS=@TE_LDFLAGS@ 



CFLAGS=@CFLAGS@ $(SNMP_F)

TESTS=parse_simple put_get parse_file
check_PROGRAMS=$(TESTS)

extra_DIST= \
put_get.c \
parse_simple.c \
parse_file.c \
oid1.c \
prn01.c \
prn02.c \
val_set01.c \
val_set02.c \
val_set03.c \
val_set04.c \
write_ind1

noinst_HEADERS=test_types.h

# link flags for tests. 
LDADD=-lasn -lndn

