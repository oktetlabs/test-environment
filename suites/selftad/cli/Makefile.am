
#EXTRA_DIST = package.dox

tetestdir=$(prefix)/@PACKAGE_NAME@/$(subdir)

dist_tetest_DATA = package.xml


AM_CPPFLAGS = -I$(DESTDIR)/$(prefix)/include \
              -I$(DESTDIR)/$(prefix)/../include

AM_CFLAGS =

AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 


tetest_PROGRAMS = clitest

LDADD = -lndn -lasn -ltapi -lrcfapi -lconfapi -lconf_oid -lipc -lpthread -llogger_ten
 
#clitest_DEPENDENCIES = ${TE_INSTALL}/i686-pc-linux-gnu/lib/librcfapi.a

tetest_DATA=${srcdir}/cli-filter.asn \
	    ${srcdir}/cli-csap.asn 
