
#EXTRA_DIST = package.dox

tetestdir=$(prefix)/@PACKAGE_NAME@/$(subdir)

dist_tetest_DATA = package.xml


AM_CFLAGS = -I$(DESTDIR)/$(prefix)/include \
            -I$(DESTDIR)/$(prefix)/../include

AM_CFLAGS =

AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 


tetest_PROGRAMS = clitest

LDADD = -lrcfapi -lipc -lpthread -lasn -lsnmp -lcrypto
clitest_LDADD = -lexpect -lrcfapi -lipc -lpthread -lasn -lsnmp -lcrypto
 
clitest_DEPENDENCIES = ${TE_INSTALL}/i686-pc-linux-gnu/lib/librcfapi.a

NDS_DIR=${TE_BASE}/storage/nds/self

clitestdir=${prefix}/selftest/cli_nds

clitest_DATA=${NDS_DIR}/cli/cli-filter.asn \
	     ${NDS_DIR}/cli/cli-csap.asn 
