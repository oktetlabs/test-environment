
EXTRA_DIST = package.dox

tetestdir=$(prefix)/@PACKAGE_NAME@/$(subdir)

dist_tetest_DATA = package.xml

tetest_DATA = tests-info.xml

tests-info.xml: $(srcdir)/*.c
	@TE_PATH@/te_tests_info.sh $(srcdir) > tests-info.xml

CLEANFILES = tests-info.xml

AM_CPPFLAGS = @TS_CPPFLAGS@

AM_CFLAGS = @TS_CFLAGS@

AM_LDFLAGS = @TS_LDFLAGS@

LDADD = @TS_LIBS@

tetest_PROGRAMS = \
	ip4_raw_send \
	ip4_send_icmp \
	ip4_send_udp \
	ip4_send_icmp_error_udp \
    ip4_send_tcp
