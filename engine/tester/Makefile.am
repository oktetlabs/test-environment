
AUTOMAKE_OPTIONS = foreign 

AM_CFLAGS=@TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include \
                      -I$(DESTDIR)/$(prefix)/../include
AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 

mybindir=$(prefix)/bin

mybin_SCRIPTS=te_tester

EXTRA_DIST=te_tester tester_config.xsd tester_defs.xsd test_package.xsd
