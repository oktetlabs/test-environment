
#EXTRA_DIST = package.dox

tetestdir=$(prefix)/@PACKAGE_NAME@/$(subdir)

dist_tetest_DATA = package.xml \
                   cli-csap.asn cli-filter.asn cli-tmpl01.asn


AM_CPPFLAGS = -I$(DESTDIR)/$(prefix)/include \
              -I$(DESTDIR)/$(prefix)/../include

AM_CFLAGS =

AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 


tetest_PROGRAMS = clitest tapi_cli_test tapi_cli_telnet tapi_cli_serial

LDADD = -ltapi -ltapi_cli -lndn -lasn -lrcfapi -lconfapi -lconf_oid \
        -llogger_ten -lipc -lpthread
 
#clitest_DEPENDENCIES = ${TE_INSTALL}/i686-pc-linux-gnu/lib/librcfapi.a
