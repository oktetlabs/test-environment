

AUTOMAKE_OPTIONS = foreign 

AM_CFLAGS=@TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include \
                      -I$(DESTDIR)/$(prefix)/../include
AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 

bin_PROGRAMS=rcftest

rcftest_SOURCES=rcftest.c

rcftest_LDADD=-lrcfapi -lipc -lpthread

rcftest_DEPENDENCIES=${TE_INSTALL}/i686-pc-linux-gnu/lib/librcfapi.a

NDS_DIR=${TE_BASE}/storage/nds

rcftestdir=${prefix}/rcftest/nds/
rcftest_DATA=${NDS_DIR}/file-csap.asn ${NDS_DIR}/file-tmpl01.asn
