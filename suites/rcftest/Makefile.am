
AM_CPPFLAGS = -I$(DESTDIR)/$(prefix)/include \
              -I$(DESTDIR)/$(prefix)/../include
AM_CFLAGS =

AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 


tetestdir=$(prefix)/@PACKAGE_NAME@/$(subdir)

NDS_DIR=${TE_BASE}/storage/nds
tetest_DATA = package.xml ${NDS_DIR}/file-csap.asn ${NDS_DIR}/file-tmpl01.asn

tetest_PROGRAMS=rcftest


rcftest_SOURCES = rcftest.c
rcftest_LDADD = -lrcfapi -llogger_ten -lipc -lpthread
rcftest_DEPENDENCIES = ${TE_INSTALL}/@host@/lib/librcfapi.a \
                       ${TE_INSTALL}/@host@/lib/liblogger_ten.a \
                       ${TE_INSTALL}/@host@/lib/libipc.a
