##
## Makefile.am for IPv6 Demo test suite - sockets package.
##

AM_CPPFLAGS = @TS_CPPFLAGS@
AM_CFLAGS   = @TS_CFLAGS@
AM_LDFLAGS  = @TS_LDFLAGS@

tetestdir=$(prefix)/@PACKAGE_NAME@/$(subdir)

dist_tetest_DATA = package.xml
EXTRA_DIST  = package.dox

tetest_PROGRAMS = send_recv \
                  opt_ipv6_only

MYLDADD = @TS_TE_LIBS@
MYDEPS = @TS_TE_DEPS@

send_recv_SOURCES = send_recv.c
send_recv_LDADD = $(MYLDADD)
send_recv_DEPENDENCIES = $(MYDEPS)

opt_ipv6_only_SOURCES = opt_ipv6_only.c
opt_ipv6_only_LDADD = $(MYLDADD)
opt_ipv6_only_DEPENDENCIES = $(MYDEPS)