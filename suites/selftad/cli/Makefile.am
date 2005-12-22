
#EXTRA_DIST = package.dox

tetestdir=$(prefix)/@PACKAGE_NAME@/$(subdir)

dist_tetest_DATA = \
    package.xml \
    cli-csap.asn \
    cli-filter.asn \
    cli-tmpl01.asn


AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@
AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(libdir)


tetest_PROGRAMS = \
    clitest \
    tapi_cli_test \
    tapi_cli_telnet \
    tapi_cli_serial \
    tapi_cli_shell

LDADD = -ltapi -ltapi_cli -lndn -lasn -lrcfapi -lrpcxdr -lconfapi \
        -lconf_oid -llogger_ten -lipc -ltools -lpthread
 
