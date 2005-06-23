AM_CPPFLAGS=@TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include
AM_CFLAGS =
AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib -L./../db -L./../rcf_emul

TESTS = test

check_PROGRAMS = $(TESTS)

noinst_HEADERS = test.h

test_SOURCES = test.c

EXTRA_DIST = config.db test.conf conf_tester.common logger.conf

test_LDADD =  -lrcf_emul -lconfapi -lconf_oid -lpthread

test_DEPENDENCIES = @TE_DEPENDENCIES@ \
                    $(DESTDIR)/$(prefix)/lib/libconfapi.a \
                    $(DESTDIR)/$(prefix)/lib/libconf_oid.a \
                    $(DESTDIR)/$(prefix)/lib/liblogger_ten.a \
                    $(DESTDIR)/$(prefix)/lib/librcfapi.a \
                    $(DESTDIR)/$(prefix)/lib/librpcxdr.a \
                    $(DESTDIR)/$(prefix)/lib/libipcserver.a \
                    $(DESTDIR)/$(prefix)/lib/libipc.a \
                    ./../rcf_emul/.libs/librcf_emul.a \
                    ./../db/.libs/libdb.a
